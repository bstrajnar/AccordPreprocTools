/**
   @file HoofSuperober.h
   @author Peter Smerkol
   @brief Contains definition of HoofSuperoberclass.
*/

#ifndef HOOFSUPEROBER_GUARD
#define HOOFSUPEROBER_GUARD

#include <HoofTypes.h>
#include <HoofWorker.h>
#include <HoofH5File.h>
#include <HoofData.h>

/**
   @class HoofSuperober
   @brief Worker object that makes super observations from DBZ and VRAD data.
*/
class HoofSuperober : public HoofWorker
{
   private:
      // members
      HoofData& _data;                      ///< Object holding info for superobing.
      HoofH5File& _outFile;                 ///< Output file to write the superobed data to.
      bool _dbzsNaN;                        ///< Flag if all DBZ data is nan.
      bool _vradsNaN;                       ///< Flag if all VRAD data is nan.
      hoof::vector2D<int> _rangeBorders;    ///< Borders of superobed range bins (nsel, nsr).
      hoof::vector3D<int> _startRayBorders; ///< Starts of superobed ray bins (nsel, nsr, nsaz).
      hoof::vector3D<int> _endRayBorders;   ///< Ends of superobed ray bins (nsel, nsr, nsaz).

      // gets superob bin borders for ranges, start rays and end rays
      void _calculateBinBorders(const std::string& type);

   public:
      // constructor
      HoofSuperober(HoofData& data, HoofH5File& outFile);
      // checks data if it is ok for superobing
      void checkData();
      // prepares the superobed metadata
      void prepareMetadata();
      // superobs data
      void superob();
      // writes superobed data to file
      void write();
};

#endif // HOOFSUPEROBER_GUARD