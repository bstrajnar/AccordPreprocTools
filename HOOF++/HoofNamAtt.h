/**
   @file HoofNamAtt.h
   @author Peter Smerkol
   @brief Contains definition of HoofNamAtt class.
*/

#ifndef HOOFNAMATT_GUARD
#define HOOFNAMATT_GUARD

#include <string>
#include <vector>
#include <optional>

/**
   @class HoofNamAtt
   @brief Class representing one radar attribute from the namelist.
*/
class HoofNamAtt
{
   public:
      // members
      std::string type;                   ///< Type of the attribute (I - integer, F - double, S - string).
      std::string group;                  ///< Group that the attribute belongs to.
      std::string name;                   ///< Name of the attribute.
      std::optional<int> iValue;          ///< Possible integer value.
      std::optional<double> dValue;       ///< Possible double value.
      std::optional<std::string> sValue;  ///< Possible string value.

      // constructor, parses a line from the namelist
      HoofNamAtt(const std::string& line);
      // get the metadata group
      std::optional<std::string> getMetadataGroup(const std::string& groupType) const;
      // equality operator so the class can be searched for in vectors
      bool operator==(const HoofNamAtt& other) const;
};

#endif // HOOFNAMATT_GUARD