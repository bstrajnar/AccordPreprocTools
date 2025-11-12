/**
   @file HoofDealiaser.h
   @author Peter Smerkol
   @brief Contains definition of HoofDealiaser class.
*/

#ifndef HOOFDEALIASER_GUARD
#define HOOFDEALIASER_GUARD

#include <string>
#include <vector>
#include <optional>
#include <HoofTypes.h>
#include <HoofWorker.h>
#include <HoofH5File.h>
#include <HoofData.h>

/**
   @class HoofDealiaser
   @brief Worker object that dealiases VRAD measurements.
*/
class HoofDealiaser : public HoofWorker
{
   private:
      // members
      HoofData& _data;                  ///< Object holding data used in dealiasing.
      HoofH5File& _outFile;             ///< Output file to write the dealiased data to.
      hoof::vector3D<double> _As;       ///< A coefficients of the torus mapping (el, az, r).
      hoof::vector3D<double> _Bs;       ///< B coefficients of the torus mapping (el, az, r).
      hoof::vector3D<double> _Ds;       ///< D coefficients of the torus mapping (el, az, r).
      std::vector<double> _cosEls;      ///< Cosines of elevation angles for faster calculation (el).
      hoof::vector2D<double> _cosAzs;   ///< Cosines of azimuth angles for faster calculation (el, az).
      hoof::vector2D<double> _sinAzs;   ///< Sines of azimuth angles for faster calculation (el, az).
      double _vnyMin;                   ///< Smallest Nyquist velocity in the file.

   public:
      // constructor
      HoofDealiaser(HoofData& data, HoofH5File& outFile);
      // checks VRAD data if it exists and is not NaN everywhere
      void checkData();
      // calculates quantities used to get the wind model
      void calculateWindModelQtys();
      // determines the height sectors in which to run the wind model
      void determineHeightSectors();
      // calculates wind models for all height sectors
      void calculateWindModels();
      // dealiases
      void dealias();
      // writes the dealiased data to the VRAD data group
      void write();
};

#endif // HOOFDEALIASER_GUARD