/**
   @file HoofNamelistAtt.cpp
   @author Peter Smerkol
   @brief Contains the HoofNamelistAtt class implementation.
*/

#include <string> 
#include <vector>
#include <iostream>
#include <optional>
#include <HoofAux.h>
#include <HoofNamAtt.h>

using std::string;
using std::vector;
using std::optional;

/**
   @brief Constructor, parses a line from the namelist.
   @param line Line from the namelist to parse.
*/
HoofNamAtt::HoofNamAtt(const string& line)
{
   // split the namelist line
   vector<string> words = HoofAux::split(line, "/", " ");

   // find the words index for "="
   int eidx = -1;
   for(int i=0; i<words.size(); i++)
   {
      if(words[i] == "=")
      {
         eidx = i;
         break;
      } 
   }

   // fill group and name
   string namGroup;
   for(int i=1; i<eidx-1; i++)
      namGroup = namGroup + "/" + words[i];
   group = namGroup;
   name = words[eidx-1];
   
   // fill value according to type
   type = words[0];
   string namValue = words.back();
   iValue = std::nullopt;
   dValue = std::nullopt;
   sValue = std::nullopt;
   if(words.back() != "None")
   {
      if(type == "S")
         sValue = namValue;
      else if(type == "I")
         iValue = HoofAux::to<int>(namValue);
      else if(type == "F")
         dValue = HoofAux::to<double>(namValue);
   }
}

optional<string> HoofNamAtt::getMetadataGroup(const string& groupType) const
{
   vector<string> groups = HoofAux::split(group, "/", " ");
   int gSize = groups.size();
   bool cond;
   if(groupType == "root")
      cond = (gSize == 1 && groups[0] != "dataset");
   if(groupType == "dataset")
      cond = (gSize == 2 && groups[1] != "data" && groups[1] != "quality");
   if(groupType == "data")
      cond = (gSize == 3 && groups[1] == "data");
   if(groupType == "quality")
      cond = (gSize == 3 && groups[1] == "quality");

   if(cond)
      return group;
   return std::nullopt;
}

// equality operator so the class can be searched for in vectors
bool HoofNamAtt::operator==(const HoofNamAtt& other) const
{
   return type == other.type && group == other.group && name == other.name &&
          sValue == other.sValue && iValue == other.iValue && dValue == other.dValue;
}
           
