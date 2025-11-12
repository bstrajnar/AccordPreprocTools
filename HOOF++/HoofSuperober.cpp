/**
   @file HoofSuperober.cpp
   @author Peter Smerkol
   @brief Contains the HoofSuperober class implementation.
*/

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <HoofTypes.h>
#include <HoofAux.h>
#include <HoofSettings.h>
#include <HoofH5File.h>
#include <HoofData.h>
#include <HoofSuperober.h>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::abs;
using std::max_element;
using std::floor;
using std::sqrt;
using std::isnan;
using namespace hoof;

/**
   @brief Constructor.
   @param data A HoofData object to fill to be used later in dealiasing and superobing.
   @param outFile The output file.
*/
HoofSuperober::HoofSuperober(HoofData& data, HoofH5File& outFile) :
   _outFile(outFile), _data(data), _dbzsNaN(false), _vradsNaN(false)
{
   classMessage = "Superobing";
}

/**
   @brief Calculates superob bin borders for ranges, start rays and end rays either for DBZ or VRAD
    
   Warning: overwrites the internal matrices that hold the borders!

   @param type "DBZ" or "VRAD".
 */
void HoofSuperober::_calculateBinBorders(const string& type)
{
   // short aliases
   int binF = HoofSettings::rangeBinFactor;
   int rayF = HoofSettings::rayAngleFactor;
   int zmax = (int)((rayF-1)/2);

   int Nsel = 0;
   if(type == "DBZ")
      Nsel = _data.sdbz.nel;
   if(type == "VRAD")
      Nsel = _data.svrad.nel;

   // clear border vectors so they are not prefilled
   _rangeBorders.clear();
   _startRayBorders.clear();
   _endRayBorders.clear();

   // loop on superobed elevations
   for(int i=0; i<Nsel; i++)
   {
      // short aliases
      int naz = 0;
      int nr = 0;
      int nsaz = 0;
      int nsr = 0;
      double rscale = 0.0;
      if(type == "DBZ")
      {
         naz = _data.dbz.naz[i];
         nr = _data.dbz.nr[i];         
         nsaz = _data.sdbz.naz[i];
         nsr = _data.sdbz.nr[i];
         rscale = _data.dbz.rscales[i];
      }

      if(type == "VRAD")
      {
         naz = _data.vrad.naz[i];
         nr = _data.vrad.nr[i];         
         nsaz = _data.svrad.naz[i];
         nsr = _data.svrad.nr[i];
         rscale = _data.vrad.rscales[i];
      }
      double L = 360.0*360.0*HoofSettings::maxArcSize/(2.0*HoofAux::Pi*(double)(naz*binF)*rscale);

      // calculate range bin borders for superob points
      vector<int> currRangeBorders;
      for(int j=0; j<nr+binF; j=j+binF)
         currRangeBorders.push_back(j);
      if(currRangeBorders.back() > nr)
         currRangeBorders.pop_back();
      _rangeBorders.push_back(currRangeBorders);

      // calculate ray bin limits and sub ray factors because of arc size limits
      vector<int> limIdxs;
      limIdxs.push_back(0);
      vector<int> facSubs;
      for(int z=0; z<=zmax; z++)
      {
         int newFac = 2*(zmax-z) + 1;
         int limIdx = floor(L/(int)newFac - 1.0) + 1;
         if(limIdx >= currRangeBorders.size())
            limIdx = currRangeBorders.size();
         if(limIdx != limIdxs.back())
            limIdxs.push_back(limIdx);
            facSubs.push_back(z);
      }
      if(limIdxs.back() < currRangeBorders.size())
         limIdxs.back() = currRangeBorders.size();

      // calculate start and end ray bin borders for superob points
      vector<int> origStartRayBorders;
      vector<int> origEndRayBorders;
      for(int j=0; j<naz; j=j+rayF)
      {
         origStartRayBorders.push_back(j);
         origEndRayBorders.push_back(rayF+j);
      }
      vector2D<int> currStartRayBorders = vector2D<int>(nsr, vector<int>(nsaz, iNaN));
      vector2D<int> currEndRayBorders = vector2D<int>(nsr, vector<int>(nsaz, iNaN));
      int currFacSub = -1;
      for(int j=0; j<nsr; j++)
      {
         for(int k=0; k<facSubs.size(); k++)
         {
            if(j >= limIdxs[k] && j < limIdxs[k+1])
               currFacSub = facSubs[k];
         }
         for(int k=0; k<nsaz; k++)
         {
            currStartRayBorders[j][k] = origStartRayBorders[k] + currFacSub;
            currEndRayBorders[j][k] = origEndRayBorders[k] - currFacSub; 
         }
      }
      _startRayBorders.push_back(currStartRayBorders);
      _endRayBorders.push_back(currEndRayBorders);
   }
}

/**
  @brief Checks if data is ok for superobing.
*/
void HoofSuperober::checkData()
{
   if(_data.dbz.nel == 0 && _data.vrad.nel == 0)
   {
         error("no data to superob");
         return;
   }

   if(HoofAux::isallnan(_data.dbz.meas))
      _dbzsNaN = true;
   if(HoofAux::isallnan(_data.vrad.meas))
      _vradsNaN = true;      
   if(_dbzsNaN && _vradsNaN)
   {
      error("all data is NaN");
   }

   if(_dbzsNaN && !_vradsNaN)
      warning("all DBZ data is NaN");
   if(!_dbzsNaN && _vradsNaN)
      warning("all VRAD data is NaN");  
}

/**
   @brief Prepares the superobed metadata.
*/
void HoofSuperober::prepareMetadata()
{
   // short aliases
   int binF = HoofSettings::rangeBinFactor;
   int rayF = HoofSettings::rayAngleFactor;

   // DBZ measurements
   int nel = _data.dbz.nel;
   _data.sdbz.naz = vector<int>(nel, iNaN);
   _data.sdbz.nr = vector<int>(nel, iNaN);
   if(nel > 0)
   {
      _data.sdbz.nel = nel;
      for(int i=0; i<nel; i++)
      {
         _data.sdbz.nr[i] = (int)(_data.dbz.nr[i] / binF);
         _data.sdbz.naz[i] = (int)(_data.dbz.naz[i] / rayF);
      }
      _data.sdbz.nrMax =  *max_element(_data.sdbz.nr.begin(), _data.sdbz.nr.end());
      _data.sdbz.nazMax =  *max_element(_data.sdbz.naz.begin(), _data.sdbz.naz.end());
      _data.sdbz.elangles = _data.dbz.elangles;
      _data.sdbz.azimuths = vector2D<double>(nel, vector<double>(_data.sdbz.nazMax, dNaN));
      _data.sdbz.ranges = vector2D<double>(nel, vector<double>(_data.sdbz.nrMax, dNaN));
      _data.sdbz.rscales = vector<double>(nel, dNaN);
      _data.sdbz.rstarts = vector<double>(nel, dNaN);
      for(int i=0; i<nel; i++)
      {
         int a = _data.sdbz.naz[i];
         int r = _data.sdbz.nr[i];
         double rstart = _data.dbz.rstarts[i];
         double rscale = _data.dbz.rscales[i];
         HoofAux::linspace( _data.sdbz.azimuths[i], 0.0, 2.0*HoofAux::Pi, a);
         _data.sdbz.rstarts[i] = rstart;
         _data.sdbz.rscales[i] = binF * rscale;
         HoofAux::linspace(_data.sdbz.ranges[i], rstart, rstart + binF*rscale*r, r);
      }
   }

   // VRAD measurements
   int nelv = _data.vrad.nel;
   _data.svrad.naz = vector<int>(nelv, iNaN);
   _data.svrad.nr = vector<int>(nelv, iNaN);
   if(nelv > 0)
   {
      _data.svrad.nel = nelv;
      for(int i=0; i<nelv; i++)
      {
         _data.svrad.nr[i] = (int)(_data.vrad.nr[i] / binF);
         _data.svrad.naz[i] = (int)(_data.vrad.naz[i] / rayF);
      }
      _data.svrad.nrMax =  *max_element(_data.svrad.nr.begin(), _data.svrad.nr.end());
      _data.svrad.nazMax =  *max_element(_data.svrad.naz.begin(), _data.svrad.naz.end());
      _data.svrad.elangles = _data.vrad.elangles;
      _data.svrad.azimuths = vector2D<double>(nelv, vector<double>(_data.svrad.nazMax, dNaN));
      _data.svrad.ranges = vector2D<double>(nelv, vector<double>(_data.svrad.nrMax, dNaN));
      _data.svrad.rscales = vector<double>(nelv, dNaN);
      _data.svrad.rstarts = vector<double>(nelv, dNaN);
      for(int i=0; i<nelv; i++)
      {
         int a = _data.svrad.naz[i];
         int r = _data.svrad.nr[i];
         double rstart = _data.vrad.rstarts[i];
         double rscale = _data.vrad.rscales[i];
         HoofAux::linspace( _data.svrad.azimuths[i], 0.0, 2.0*HoofAux::Pi, a);
         _data.svrad.rstarts[i] = rstart;
         _data.svrad.rscales[i] = binF * rscale;
         HoofAux::linspace(_data.svrad.ranges[i], rstart, rstart + binF*rscale*r, r);
      }
   }
}

/**
   @brief Superobs the data.
*/
void HoofSuperober::superob()
{
   // short aliases
   double arcmax = HoofSettings::maxArcSize;
   double clearth = HoofSettings::dbzClearsky;
   double qualth = HoofSettings::minQuality;
   double dbzgood = HoofSettings::dbzPercentage;
   double vradgood = HoofSettings::vradPercentage;
   double maxstd = HoofSettings::vradMaxStd;
   double dbzmin = HoofAux::nanminmax(_data.dbz.meas)[0];
   int binF = HoofSettings::rangeBinFactor;
   int rayF = HoofSettings::rayAngleFactor;
   int zmax = (int)((rayF-1)/2);
   int Nel = _data.dbz.nel;
   int Naz = _data.dbz.nazMax;
   int Nr = _data.dbz.nrMax;
   int Nsel = _data.sdbz.nel;
   int Nsaz = _data.sdbz.nazMax;
   int Nsr = _data.sdbz.nrMax;
   int Nelv = _data.vrad.nel;
   int Nazv = _data.vrad.nazMax;
   int Nrv = _data.vrad.nrMax;
   int Nselv = _data.svrad.nel;
   int Nsazv = _data.svrad.nazMax;
   int Nsrv = _data.svrad.nrMax;       

   // superob DBZ measurements
   if(Nel > 0)
   {
      // calculate superobed bin borders
      _calculateBinBorders("DBZ");

      // prepare the superobed arrays
      _data.sdbz.meas = vector3D<double>(Nsel, vector2D<double>(Nsaz, vector<double>(Nsr, dNaN)));
      _data.sdbz.ths = vector3D<double>(Nsel, vector2D<double>(Nsaz, vector<double>(Nsr, dNaN)));
      _data.sdbz.quals = vector3D<double>(Nsel, vector2D<double>(Nsaz, vector<double>(Nsr, dNaN)));

      // roll the original arrays by zmax to get the correct ray positions
      vector3D<double> meas = vector3D<double>(Nel, vector2D<double>(Naz, vector<double>(Nr, dNaN)));
      vector3D<double> ths = vector3D<double>(Nel, vector2D<double>(Naz, vector<double>(Nr, dNaN)));
      vector3D<double> quals = vector3D<double>(Nel, vector2D<double>(Naz, vector<double>(Nr, 0.0)));
      for(int i=0; i<Nel; i++)
      {
         for(int j=0; j<Naz; j++)
         {
            int newj = j + zmax >= Naz ? j + zmax - Naz : j + zmax;
            for(int k=0; k<Nr; k++)
            {
               meas[i][newj][k] = _data.dbz.meas[i][j][k];
               ths[i][newj][k] = _data.dbz.ths[i][j][k];
               quals[i][newj][k] = _data.dbz.quals[i][j][k];
            }
         }
      }

      // loop on superobed elevations
      for(int i=0; i<Nsel; i++)
      {
         // short aliases
         int naz = _data.dbz.naz[i];
         int nr = _data.dbz.nr[i];         
         int nsaz = _data.sdbz.naz[i];
         int nsr = _data.sdbz.nr[i];

         // make superobs
         for(int j=0; j<nsr; j++)
         {
            int startBin = _rangeBorders[i][j];
            int endBin = _rangeBorders[i][j+1];
            for(int k=0; k<nsaz; k++)
            {
               int startRay = _startRayBorders[i][j][k];
               int endRay = _endRayBorders[i][j][k];

               // count the wet and dry points and calculate average of wet points
               int nWet = 0;
               int nDry = 0;
               double wetAvg = 0.0;
               int nWetTh = 0;
               double wetAvgTh = 0.0;
               for(int l=startRay; l<endRay; l++)
               {
                  for(int m=startBin; m<endBin; m++)
                  {
                     double d = meas[i][l][m];
                     double t = ths[i][l][m];
                     double q = quals[i][l][m];
                     if(q > qualth)
                     {
                        if(d > clearth)
                        {
                           nWet++;
                           wetAvg = wetAvg + d;
                           if(t < 100000.0)
                           {
                              nWetTh++;
                              wetAvgTh = wetAvgTh + t;
                           }
                        }
                        else
                           nDry++;
                     }
                  }
               }

               // calculate and store the superob
               if(nWet > dbzgood * (double)((endRay-startRay)*(endBin-startBin)))
               {
                  _data.sdbz.meas[i][k][j] = wetAvg/(double)nWet;
                  if(nWetTh > 0)
                     _data.sdbz.ths[i][k][j] = wetAvgTh/(double)nWetTh;
                  _data.sdbz.quals[i][k][j] = 1.0;
               }
               else
               {
                  if(nDry > 0)
                  {
                     _data.sdbz.meas[i][k][j] = dbzmin;
                     _data.sdbz.quals[i][k][j] = 1.0;
                  }
               }

            }
         }
      }
   }

   // superob VRAD measurements
   if(Nelv > 0)
   {
      // calculate superobed bin borders
      _calculateBinBorders("VRAD");

      // prepare the superobed arrays
      _data.svrad.meas = vector3D<double>(Nselv, vector2D<double>(Nsazv, vector<double>(Nsrv, dNaN)));
      _data.svrad.quals = vector3D<double>(Nselv, vector2D<double>(Nsazv, vector<double>(Nsrv, 0.0)));

      // roll the original array by zmax to get the correct ray positions
      vector3D<double> meas = vector3D<double>(Nelv, vector2D<double>(Nazv, vector<double>(Nrv, dNaN)));
      vector3D<double> oldmeas;
      if(HoofSettings::dealiasing)
         oldmeas = _data.dvrads;
      else
         oldmeas = _data.vrad.meas;
      for(int i=0; i<Nelv; i++)
      {
         for(int j=0; j<Nazv; j++)
         {
            int newj = j + zmax >= Nazv ? j + zmax - Nazv : j + zmax;
            for(int k=0; k<Nrv; k++)
               meas[i][newj][k] = oldmeas[i][j][k];
         }
      }

      // loop on superobed elevations
      for(int i=0; i<Nselv; i++)
      {
         // short aliases
         int naz = _data.vrad.naz[i];
         int nr = _data.vrad.nr[i];         
         int nsaz = _data.svrad.naz[i];
         int nsr = _data.svrad.nr[i];

         // make superobs
         for(int j=0; j<nsr; j++)
         {
            int startBin = _rangeBorders[i][j];
            int endBin = _rangeBorders[i][j+1];
            for(int k=0; k<nsaz; k++)
            {
               int startRay = _startRayBorders[i][j][k];
               int endRay = _endRayBorders[i][j][k];         
            

               // count the good points and calculate average and standard deviation
               int nGood = 0;
               double s = 0.0;
               double s2 = 0.0;
               double avg = 0.0;
               double std = maxstd + 1.0;
               for(int l=startRay; l<endRay; l++)
               {
                  for(int m=startBin; m<endBin; m++)
                  {
                     double v = meas[i][l][m];
                     if(v < 1000000.0)
                     {
                        nGood++;
                        s = s + m;
                        s2 = s2 + m*m;
                     }

                  }
               }
               if(nGood > 0)
               {
                  avg = s / (double)nGood;
                  double std2 = (s2 - s*avg)/(double)nGood;
                  std = std2 > 0.0 ? sqrt(std2) : 0.0;
               }

               // calculate and store the superob
               if(nGood > vradgood*(double)((endRay-startRay)*(endBin-startBin)) && std < maxstd)
               {
                  _data.svrad.meas[i][k][j] = avg;
                  _data.svrad.quals[i][k][j] = 1.0;
               }
            }
         }
      }
   }
}

/**
   @brief Writes superobed data to file.
*/
void HoofSuperober::write()
{
   // write DBZ superobed data
   for(int i=0; i<_data.dbz.datasets.size(); i++)
   {
      string dataset = _data.dbz.datasets[i];

      // get the attributes and 2D data arrays for current elevation
      int naz = _data.sdbz.naz[i];
      int nr = _data.sdbz.nr[i];
      double rscale = _data.sdbz.rscales[i];
      double nodataDbz = _outFile.getAtt<double>(dataset + "/data1/what", "nodata").value();
      double nodataTh = _outFile.getAtt<double>(dataset + "/data2/what", "nodata").value();
      vector2D<double> elDbz(naz, vector<double>(nr, dNaN));
      vector2D<double> elTh(naz, vector<double>(nr, dNaN));
      vector2D<double> elQual(naz, vector<double>(nr, dNaN));
      for(int j=0; j<naz; j++)
      {
         for(int k=0; k<nr; k++)
         {
            elDbz[j][k] = _data.sdbz.meas[i][j][k];
            elTh[j][k] = _data.sdbz.ths[i][j][k];
            elQual[j][k] = _data.sdbz.quals[i][j][k];
         }           
      }

      // prepare data to write
      double gainDbz = 1.0;
      double offsetDbz = 0.0;
      double gainTh = 1.0;
      double offsetTh = 0.0;      
      double gainQual = 1.0/255.0;
      double offsetQual = 0.0;
      if(!HoofAux::isallnan(elDbz))
      {
         Tuple minmax = HoofAux::nanminmax(elDbz);
         gainDbz = (minmax[1]-minmax[0]) / 254.0;
         if(HoofAux::eqDbl(gainDbz, 0.0))
            gainDbz = 1.0;
         offsetDbz = (254.0 * minmax[0] - minmax[1]) / 253.0;
      }
      if(!HoofAux::isallnan(elTh))
      {
         Tuple minmax = HoofAux::nanminmax(elTh);
         gainTh = (minmax[1]-minmax[0]) / 254.0;
         if(HoofAux::eqDbl(gainTh, 0.0))
            gainTh = 1.0;
         offsetTh = (254.0 * minmax[0] - minmax[1]) / 253.0;
      }
      unsigned char nodataRawDbz = static_cast<unsigned char>(nodataDbz);
      unsigned char nodataRawTh = static_cast<unsigned char>(nodataTh);      
      vector2D<unsigned char> dataDbz(naz, vector<unsigned char>(nr, nodataRawDbz));
      vector2D<unsigned char> dataTh(naz, vector<unsigned char>(nr, nodataRawTh));
      vector2D<unsigned char> dataQual(naz, vector<unsigned char>(nr, 0));
      for(int j=0; j<naz; j++)
      {
         for(int k=0; k<nr; k++)
         {
            if(!isnan(elDbz[j][k]))
               dataDbz[j][k] = static_cast<unsigned char>((elDbz[j][k] - offsetDbz + 0.5*gainDbz) / gainDbz); 
            if(!isnan(elTh[j][k]))
               dataTh[j][k] = static_cast<unsigned char>((elTh[j][k] - offsetTh + 0.5*gainTh) / gainTh);
            if(!isnan(elQual[j][k]))
               dataQual[j][k] = static_cast<unsigned char>((elQual[j][k] - offsetQual + 0.5*gainQual) / gainQual);                              
         }            
      }

      // write to file, overwriting the original DBZ and TH measurements
      // and writing the superob quality group to quality1 group
      _outFile.writeAtt<double>(dataset + "/where", "nbins", nr);
      _outFile.writeAtt<double>(dataset + "/where", "nrays", naz);
      _outFile.writeAtt<double>(dataset + "/where", "rscale", rscale);
      _outFile.writeAtt<double>(dataset + "/data1/what", "undetect", 0.0);
      _outFile.writeAtt<double>(dataset + "/data1/what", "gain", gainDbz);
      _outFile.writeAtt<double>(dataset + "/data1/what", "offset", offsetDbz);
      _outFile.writeAtt<double>(dataset + "/data2/what", "gain", gainTh);
      _outFile.writeAtt<double>(dataset + "/data2/what", "offset", offsetTh);      
      _outFile.writeAtt<double>(dataset + "/quality1/what", "gain", gainQual);
      _outFile.writeAtt<double>(dataset + "/quality1/what", "offset", offsetQual);
      _outFile.writeAtt<string>(dataset + "/quality1/how", "task", "superobing");  
      _outFile.writeDataset(dataset + "/data1", "data", dataDbz);
      _outFile.writeDataset(dataset + "/data2", "data", dataTh);
      _outFile.writeDataset(dataset + "/quality1", "data", dataQual);
   }

   // write VRAD superobed data
   for(int i=0; i<_data.vrad.datasets.size(); i++)
   {
      string dataset = _data.vrad.datasets[i];

      // get the attributes and 2D data arrays for current elevation
      int naz = _data.svrad.naz[i];
      int nr = _data.svrad.nr[i];
      double rscale = _data.svrad.rscales[i];
      vector2D<double> elVrad(naz, vector<double>(nr, dNaN));
      vector2D<double> elQual(naz, vector<double>(nr, dNaN));      
      for(int j=0; j<naz; j++)
      {
         for(int k=0; k<nr; k++)
         {
            elVrad[j][k] = _data.svrad.meas[i][j][k];
            elQual[j][k] = _data.svrad.quals[i][j][k];
         }           
      }

      // prepare data to write
      double gainVrad = 1.0;
      double offsetVrad = 0.0;
      double gainQual = 1.0/255.0;
      double offsetQual = 0.0;
      if(!HoofAux::isallnan(elVrad))
      {
         Tuple minmax = HoofAux::nanminmax(elVrad);
         gainVrad = (minmax[1]-minmax[0]) / 254.0;
         if(HoofAux::eqDbl(gainVrad, 0.0))
            gainVrad = 1.0;
         offsetVrad = (254.0 * minmax[0] - minmax[1]) / 253.0;
      }
      vector2D<unsigned char> dataVrad(naz, vector<unsigned char>(nr, 255.0));
      vector2D<unsigned char> dataQual(naz, vector<unsigned char>(nr, 0));
      for(int j=0; j<naz; j++)
      {
         for(int k=0; k<nr; k++)
         {
            if(!isnan(elVrad[j][k]))
               dataVrad[j][k] = static_cast<unsigned char>((elVrad[j][k] - offsetVrad + 0.5*gainVrad) / gainVrad); 
            if(!isnan(elQual[j][k]))
               dataQual[j][k] = static_cast<unsigned char>((elQual[j][k] - offsetQual + 0.5*gainQual) / gainQual);                              
         }            
      }

      // write to file
      _outFile.writeAtt<double>(dataset + "/where", "nbins", nr);
      _outFile.writeAtt<double>(dataset + "/where", "nrays", naz);
      _outFile.writeAtt<double>(dataset + "/where", "rscale", rscale);
      _outFile.writeAtt<double>(dataset + "/data1/what", "undetect", 0.0);
      _outFile.writeAtt<double>(dataset + "/data1/what", "gain", gainVrad);
      _outFile.writeAtt<double>(dataset + "/data1/what", "offset", offsetVrad);
      _outFile.writeAtt<double>(dataset + "/data1/what", "nodata", 255.0);
      _outFile.writeAtt<double>(dataset + "/data1/what", "undetect", 0.0);
      _outFile.writeAtt<double>(dataset + "/quality1/what", "gain", gainQual);
      _outFile.writeAtt<double>(dataset + "/quality1/what", "offset", offsetQual);
      _outFile.writeAtt<string>(dataset + "/quality1/how", "task", "superobing"); 
      _outFile.writeDataset(dataset + "/data1", "data", dataVrad);
      _outFile.writeDataset(dataset + "/quality1", "data", dataQual);           
   }
}
