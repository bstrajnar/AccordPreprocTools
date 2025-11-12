/**
   @file HoofSettings.h
   @author Peter Smerkol
   @brief Contains definition of HoofSettings class.
*/

#ifndef HOOFSETTINGS_GUARD
#define HOOFSETTINGS_GUARD

#include <map>
#include <string>
#include <vector>
#include <HoofTypes.h>
#include <HoofNamAtt.h>

/**
   @class HoofSettings
   @brief Static class that holds all settings read from the namelist.
*/
class HoofSettings
{
   public:
      // constructor, parses given namelist
      HoofSettings(const std::string& _namelist, const std::string& _inFolder, const std::string& _outFolder);
        
      // members
      static std::string inFolder;                    ///< Relative path to folder with input files
      static std::string outFolder;                   ///< Relative path to folder for output files
      static std::string namelist;                    ///< Name of the namelist file
      static std::vector<std::string> fileExtensions; ///< File extensions representing valid radar files
      static std::string warningTag;                  ///< Text printed next to warnings, to make them searchable
      static std::string errorTag;                    ///< Text printed next to errors, to make them searchable
      static bool printConsoleWarnings;               ///< Flag for writing warnings to console
      static bool printLogWarnings;                   ///< Flag for writing warnings to log
      static bool printConsoleErrors;                 ///< Flag for writing errors to console
      static bool printConsoleTiming;                 ///< Flag for writing timing to console
      static std::vector<std::string> dbzNames;       ///< Radar moment names containing DBZ
      static std::vector<std::string> thNames;        ///< Radar moment names containing TH
      static std::vector<std::string> vradNames;      ///< Radar moment names containing VRAD
      static std::vector<std::string> dbzQualNames;   ///< Quality groups attached to DBZ to keep
      static std::vector<HoofNamAtt> comAtts;         ///< Common radar attributes
      static hoof::VecDict<HoofNamAtt> specAtts;      ///< Specific radar attributes
      static bool dealiasing;                         ///< Flag for dealiasing
      static double zSectorSize;                      ///< Size of z bin sector in wind model calculation
      static double zMax;                             ///< Maximum height to dealias
      static int minGoodPoints;                       ///< Minimum number of good points in a sector for wind model
      static double maxWind;                          ///< Maximum wind speed in m/s allowed after dealiasing
      static bool superobing;                         ///< Flag for superobing
      static int rangeBinFactor;                      ///< Range bin multiplication factor for superobing bins
      static int rayAngleFactor;                      ///< Ray angle multiplication factor for superobing bins
      static double maxArcSize;                       ///< Maximum allowed arc size in meters for superob bins
      static double minQuality;                       ///< Minimum quality of bin to be accepted in superobing
      static double dbzClearsky;                      ///< DBZ threshold for clear sky
      static double dbzPercentage;                    ///< Percentage of good points needed for superob bin in DBZ
      static double vradPercentage;                   ///< Percentage of good points needed for superob bin in VRAD
      static double vradMaxStd;                       ///< Maximum allowed standard deviation of points for superob bins for VRAD
};

#endif // HOOFSETTINGS_GUARD
