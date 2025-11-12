/**
   @file HoofDealiaser.cpp
   @author Peter Smerkol
   @brief Contains the HoofDealiaser class implementation.
*/

#include <string>
#include <vector>
#include <array>
#include <set>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <HoofTypes.h>
#include <HoofAux.h>
#include <HoofSettings.h>
#include <HoofH5File.h>
#include <HoofData.h>
#include <HoofDealiaser.h>

using std::string;
using std::vector;
using std::array;
using std::set;
using std::cos;
using std::sin;
using std::cout;
using std::endl;
using std::abs;
using std::min_element;
using std::isnan;
using namespace hoof;

/**
   @brief Constructor.
   @param data A HoofData object to fill to be used later in dealiasing and superobing.
   @param outFile The output file.
*/
HoofDealiaser::HoofDealiaser(HoofData& data, HoofH5File& outFile) :
   _outFile(outFile), _data(data)
{
   classMessage = "Dealiasing";
}

/**
   @brief Checks VRAD data if it exists and is not NaN everywhere.
*/
void HoofDealiaser::checkData()
{
   if(_data.vrad.datasets.size() == 0)
      error("no VRAD datasets in file");
   
   if(HoofAux::isallnan(_data.vrad.meas))
      error("all data in VRAD datasets are NaN");
}

/**
   @brief Calculates quantities used in minimzation to get the wind model.
*/
void HoofDealiaser::calculateWindModelQtys()
{
   // initialize with NaNs
   int nel = _data.vrad.nel;
   int naz = _data.vrad.nazMax;
   int nr = _data.vrad.nrMax;
   double Pi = HoofAux::Pi;
   _As = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
   _Bs = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
   _Ds = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
   _cosEls = vector<double>(nel, dNaN);
   _cosAzs = vector2D<double>(nel, vector<double>(naz, dNaN));
   _sinAzs = vector2D<double>(nel, vector<double>(naz, dNaN));   
   vector3D<double> f3s(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));

   // calculate A, B and F3 quantities and get the minimum Nyquist velocity
   _vnyMin = std::numeric_limits<double>::infinity();
   for(int i=0; i<nel; i++)
   {
      _cosEls[i] = cos(_data.vrad.elangles[i]);
      double vNy = _data.vrad.vnys[i];
      if(vNy < _vnyMin)
         _vnyMin = vNy;
      for(int j=0; j<_data.vrad.naz[i]; j++)
      {
         double az = _data.vrad.azimuths[i][j];
         _cosAzs[i][j] = cos(az);
         _sinAzs[i][j] = sin(az);
         for(int k=0; k<_data.vrad.nr[i]; k++)
         {
            double meas = _data.vrad.meas[i][j][k];
            _As[i][j][k] = _cosEls[i]*_cosAzs[i][j]*sin(Pi*meas/vNy);
            _Bs[i][j][k] = _cosEls[i]*_sinAzs[i][j]*sin(Pi*meas/vNy);
            f3s[i][j][k] = vNy*cos(Pi*meas/vNy)/Pi;
         }  
      }
   }

   // calculate D quantity
   for(int i=0; i<nel; i++)
   {
      int azSize = _data.vrad.naz[i];
      for(int j=0; j<azSize; j++)
      {
         // roll the azimuth and f3 matrices for derivative calculation
         int nextj = j+1 == azSize ? 0 : j+1;
         int prevj = j-1 == -1 ? azSize-1 : j-1;
         double daz = _data.vrad.azimuths[i][nextj] - _data.vrad.azimuths[i][prevj];
         if(j == 0 || j == azSize-1)
            daz = daz - 2*Pi;
         // calculate D from derivative   
         for(int k=0; k<_data.vrad.nr[i]; k++)
            _Ds[i][j][k] = (f3s[i][nextj][k] - f3s[i][prevj][k])/daz; 
      }
   }   
}

/**
   @brief Determines the height sectors in which to run the wind model.
*/
void HoofDealiaser::determineHeightSectors()
{
   // prepare variables
   double dz = HoofSettings::zSectorSize;
   double zdatamax = HoofAux::nanminmax(_data.vrad.zs)[1];
   double zmax = zdatamax < HoofSettings::zMax ? zdatamax : HoofSettings::zMax;
   double zstart = _data.height;

   // determine height sectors
   int nl = (int)((zmax-zstart)/dz)+1;
   _data.zStarts = vector<double>(nl, 0.0);
   _data.zEnds = vector<double>(nl, 0.0);
   _data.zIdxs = vector2D<Triple>(nl, vector<Triple>());
   for(int n=0; n<nl; n++)
   {
      _data.zStarts[n] = _data.height + (double)n*dz;
      _data.zEnds[n] = _data.zStarts[n] + dz;
   }
   for(int i=0; i<_data.vrad.nel; i++)
   {
      for(int j=0; j<_data.vrad.naz[i]; j++)
      {
         for(int k=0; k<_data.vrad.nr[i]; k++)
         {
            double z = _data.vrad.zs[i][j][k];
            if(!(isnan(z) || isnan(_data.vrad.meas[i][j][k]) || isnan(_Ds[i][j][k])) && z < zmax)
            {
               int idx = (int)((z-zstart)/dz);
               _data.zIdxs[idx].push_back({i,j,k});
            }
         }
      }
   }
}

/**
   @brief Calculates wind models for all height sectors.
*/
void HoofDealiaser::calculateWindModels()
{
   // prepare variables
   int nel = _data.vrad.nel;
   int naz = _data.vrad.nazMax;
   int nr = _data.vrad.nrMax;
   double vmax = HoofSettings::maxWind;
   _data.wModels = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));

   // loop on height sectors
   for(int z=0; z<_data.zIdxs.size(); z++)
   {
      vector<Triple> idxs = _data.zIdxs[z];
      int nidxs = idxs.size();

      // only calculate wind model if we have enough points in the height level
      if(nidxs >= HoofSettings::minGoodPoints)
      {
         // get the A, B and D for current height level
         double As[nidxs];
         double Bs[nidxs];
         double Ds[nidxs];
         HoofAux::subset(_As, idxs, As, nidxs);
         HoofAux::subset(_Bs, idxs, Bs, nidxs);
         HoofAux::subset(_Ds, idxs, Ds, nidxs);

         // use gsl_multifit to fit the curve to A,B and D and get u and v of the wind model
         gsl_matrix *X = gsl_matrix_alloc(nidxs, 2);
         gsl_vector *y = gsl_vector_alloc(nidxs);
         gsl_vector *c = gsl_vector_alloc(2);
         gsl_matrix *cov = gsl_matrix_alloc(2, 2);
         for(int j=0; j<nidxs; j++)
         {
            gsl_matrix_set(X, j, 0, -As[j]);
            gsl_matrix_set(X, j, 1, Bs[j]);
            gsl_vector_set(y, j, Ds[j]);
         }
         double chisq;
         gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(nidxs, 2);
         gsl_multifit_linear(X, y, c, cov, &chisq, work);
         double u = gsl_vector_get(c, 0);
         double v = gsl_vector_get(c, 1);

         for(int i=0; i<nidxs; i++)         
         {
            int iel = idxs[i][0];
            int iaz = idxs[i][1];
            double vm = _cosEls[iel] * (u * _sinAzs[iel][iaz] + v * _cosAzs[iel][iaz]);
            if(abs(vm) < vmax)
               _data.wModels[iel][iaz][idxs[i][2]] = vm;
         }
      }
   }
}

/**
   @brief Dealiases.
*/
void HoofDealiaser::dealias()
{
   // prepare variables
   int nel = _data.vrad.nel;
   int naz = _data.vrad.nazMax;
   int nr = _data.vrad.nrMax;
   double nymax = (int)(HoofSettings::maxWind/_vnyMin);
   _data.dvrads = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
   double inf = std::numeric_limits<double>::infinity();
   vector3D<double> mns = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, inf)));
   vector3D<int> ns = vector3D<int>(nel, vector2D<int>(naz, vector<int>(nr, iNaN)));

   // get Nyquist multipliers
   for(int n=-nymax; n<=nymax; n++)
   {
      for(int i=0; i<nel; i++)
      {
         double vny = _data.vrad.vnys[i];
         for(int j=0; j<_data.vrad.naz[i]; j++)
         {
            for(int k=0; k<_data.vrad.nr[i]; k++)
            {
               double wm = _data.wModels[i][j][k];
               double m = _data.vrad.meas[i][j][k];
               if(!(isnan(wm) || isnan(m)))
               {
                  double currMn = abs(m + 2.0*vny*(double)n - wm);
                  if(currMn < mns[i][j][k])
                  {
                     mns[i][j][k] = currMn;
                     ns[i][j][k] = n;
                  }
               }
            }
         }         
      }
   }

   // dealias VRAD
   for(int i=0; i<nel; i++)
   {
      double vny = _data.vrad.vnys[i];
      for(int j=0; j<_data.vrad.naz[i]; j++)
      {
         for(int k=0; k<_data.vrad.nr[i]; k++)
         {
            double m = _data.vrad.meas[i][j][k];
            int n = ns[i][j][k];
            if(!(isnan(m) || isnan(n) || isnan(_Ds[i][j][k])))
               _data.dvrads[i][j][k] = m + 2.0*(double)n*vny;
         }
      }         
   }
}

/**
   @brief Writes dealiased data to the VRAD data group.
*/
void HoofDealiaser::write()
{
   for(int i=0; i<_data.vrad.datasets.size(); i++)
   {
      string dataset = _data.vrad.datasets[i];

      // get the 2D data array for current elevation
      int naz = _data.vrad.naz[i];
      int nr = _data.vrad.nr[i];
      vector2D<double> eldata(naz, vector<double>(nr, dNaN));
      for(int j=0; j<naz; j++)
      {
         for(int k=0; k<nr; k++)
            eldata[j][k] = _data.dvrads[i][j][k];
      }

      // prepare data to write
      double gain = 1.0;
      double offset = 0.0;
      double nodata = _outFile.getAtt<double>(dataset + "/data1/what", "nodata").value();
      if(!HoofAux::isallnan(eldata))
      {
         Tuple minmax = HoofAux::nanminmax(eldata);
         gain = (minmax[1]-minmax[0]) / 254.0;
         if(HoofAux::eqDbl(gain, 0.0))
            gain = 1.0;
         offset = (254.0 * minmax[0] - minmax[1]) / 253.0;
      }
      unsigned char nodataRaw = static_cast<unsigned char>(nodata);
      vector2D<unsigned char> data(naz, vector<unsigned char>(nr, nodataRaw));
      vector2D<unsigned char> qual(naz, vector<unsigned char>(nr, static_cast<unsigned char>(0.5)));
      for(int j=0; j<naz; j++)
      {
         for(int k=0; k<nr; k++)
         {
            if(!isnan(eldata[j][k]))
            {
               data[j][k] = static_cast<unsigned char>((eldata[j][k] - offset + 0.5*gain) / gain); 
               qual[j][k] = static_cast<unsigned char>(1.5);
            }
         }            
      }
      double gainQual = 1.0/255.0;
      double offsetQual = 0.0;

      // write to file, overwriting the original VRAD measurements
      _outFile.writeAtt<double>(dataset + "/data1/what", "gain", gain);
      _outFile.writeAtt<double>(dataset + "/data1/what", "offset", offset);
      _outFile.writeDataset(dataset + "/data1", "data", data);
      _outFile.writeAtt<double>(dataset + "/quality1/what", "gain", 1.0/255.0);
      _outFile.writeAtt<double>(dataset + "/quality1/what", "offset", 0.0);
      _outFile.writeAtt<string>(dataset + "/quality1/how", "task", "dealiasing");
      _outFile.writeDataset(dataset + "/quality1", "data", qual);
   }
}