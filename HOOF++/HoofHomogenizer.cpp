/**
   @file HoofHomogenizer.cpp
   @author Peter Smerkol
   @brief Contains the HoofHomogenizer class implementation.
*/

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <optional>
#include <cctype>
#include <cmath>
#include <type_traits>
#include <limits>
#include <HoofTypes.h>
#include <HoofAux.h>
#include <HoofSettings.h>
#include <HoofH5File.h>
#include <HoofNamAtt.h>
#include <HoofData.h>
#include <HoofHomogenizer.h>
#include <iostream>
using std::cout;
using std::endl;

using std::string;
using std::vector;
using std::map;
using std::optional;
using std::find;
using std::isdigit;
using std::remove_if;
using std::is_same_v;
using std::max_element;
using std::numeric_limits;
using std::sqrt;
using std::sin;
using namespace hoof;

/**
   @brief Constructor.
   @param inFile The input file.
   @param outFile The output file.
   @param data A HoofData object to fill for further use.
*/
HoofHomogenizer::HoofHomogenizer(HoofH5File& inFile, HoofH5File& outFile, HoofData& data) :
   _inFile(inFile), _outFile(outFile), _data(data)
{
   classMessage = "Homogenization";
}

/** 
   @brief Gets the unique namelist metadata groups by group type.
   @param groupType "root", "dataset", "data" or "quality".
   @return A vector of found unique namelist metadata groups.
*/
vector<string> HoofHomogenizer::_getNamelistMetadataGroups(const string& groupType) const
{
   vector<string> metaGroups;

   // first get all metadata groups in common attributes
   for(int i=0; i<HoofSettings::comAtts.size(); i++)
   {
      optional<string> metaGroup = HoofSettings::comAtts[i].getMetadataGroup(groupType);
      if(metaGroup)
      {
         if(!HoofAux::find<string>(metaGroup.value(), metaGroups))
            metaGroups.push_back(metaGroup.value());
      }
   }  

   // then check the specific attributes if there is a new group defined here
   VecDict<HoofNamAtt>::iterator it = HoofSettings::specAtts.find(_data.site);
   if(it != HoofSettings::specAtts.end())
   {
      vector<HoofNamAtt> specAtts = it->second;
      for(int i=0; i<specAtts.size(); i++)
      {
         HoofNamAtt att = specAtts[i];
         optional<string> metaGroup = HoofSettings::comAtts[i].getMetadataGroup(groupType);
         if(metaGroup)
         {
            if(!HoofAux::find<string>(metaGroup.value(), metaGroups))
               metaGroups.push_back(metaGroup.value());
         }
      }
   }

   return metaGroups;
}

/**
   @brief Gets namelist attributes for a namelist group.
   @param groups The namelist group.
   @return The found namelist attributes.
*/
vector<HoofNamAtt> HoofHomogenizer::_getNamelistGroupAtts(const string& group) const
{
   vector<HoofNamAtt> atts;

   // first search the common attributes
   for(int i=0; i<HoofSettings::comAtts.size(); i++)
   {
      HoofNamAtt att = HoofSettings::comAtts[i];
      if(att.group == group)
         atts.push_back(att);
   }

   // then search for new ones in the specific attributes
   VecDict<HoofNamAtt>::iterator it = HoofSettings::specAtts.find(_data.site);
   if(it != HoofSettings::specAtts.end())
   {
      vector<HoofNamAtt> specAtts = it->second;
      for(int i=0; i<specAtts.size(); i++)
      {
         HoofNamAtt att = specAtts[i];
         if(att.group == group && !HoofAux::find(att, atts))
               atts.push_back(att);
      }
   }

   return atts;
}   

/**
   @private
   @brief Gets an attribute value of type T either from namelist or the input file.
   @param group The attribute group.
   @param name The attribute name.
   @return The attribute value or std::nullopt if not found anywhere.
*/
template<typename T> optional<T> HoofHomogenizer::_getAtt(const string& group, const string& name)
{
   // check if the requested attribute exists in the input or output file and return it if it exists
   optional<T> fileAttValue;
   fileAttValue = _inFile.getAtt<T>(group, name);
   if(fileAttValue)
      return fileAttValue;

   // remove digits from the group so it can be searched for in the namelist attributes
   string trGroup = HoofAux::removeDigits(group);

   // next, check specific attributes if attribute exists and has value and return it if it is ok
   optional<T> specValue = std::nullopt;
   VecDict<HoofNamAtt>::iterator it = HoofSettings::specAtts.find(_data.site);
   if(it != HoofSettings::specAtts.end())
   {
      vector<HoofNamAtt> specAtts = it->second;
      for(int j=0; j<specAtts.size(); j++)
      {
         HoofNamAtt specAtt = specAtts[j];
         if(specAtt.group == trGroup && specAtt.name == name)
         {
            if constexpr (is_same_v<T, string>)
               specValue = specAtt.sValue;
            else if constexpr (is_same_v<T, double>)
               specValue = specAtt.dValue;
            else if constexpr (is_same_v<T, int>)
               specValue = specAtt.iValue;
            break;
         }
      }
   }
   if(specValue)
      return specValue;

   // finally check common attributes if the attribute exists and has value and return it if it is ok
   optional<T> comValue = std::nullopt;
   for(int i=0; i<HoofSettings::comAtts.size(); i++)
   {
      HoofNamAtt comAtt = HoofSettings::comAtts[i];
      if(comAtt.group == trGroup && comAtt.name == name)
      {
         if constexpr (is_same_v<T, string>)
            comValue = comAtt.sValue;
         else if constexpr (is_same_v<T, double>)
            comValue = comAtt.dValue;
         else if constexpr (is_same_v<T, int>)
            comValue = comAtt.iValue;
         break;
      }
   }
   if(comValue)
      return comValue;
   
   // if nothing is found, add an error and return std::nullopt
   error("attribute " + group + "/" + name + " not found");
   return std::nullopt;
}

/**
   @private  
   @brief Gets an attribute value of type T from the homogenized file.
   @param group The attribute group.
   @param name The attribute name.
   @return The attribute value or std::nullopt if not found anywhere.
*/
template<typename T> optional<T> HoofHomogenizer::_getHomAtt(const string& group, const string& name)
{
   optional<T> fileAttValue = _outFile.getAtt<T>(group, name);
   if(!fileAttValue)
      error("attribute " + group + "/" + name + " not found in the homogenized file");
   return fileAttValue;
}

/**
   @brief Gets the elevation angle of a dataset rounded to 1 decimal.
   @param dataset Dataset name.
   @return The angle rounded to one decimal place or std::nullopt if it does not exist.
*/ 
optional<double> HoofHomogenizer::_getRoundedElAngle(const string& dataset)
{
   // the elangle attribute is in the where group of the dataset
   string angleGroup = dataset + "/where";
   optional<double> elAngle = _getAtt<double>(angleGroup, "elangle");

   if(elAngle)
      elAngle = HoofAux::round(elAngle.value(), 0.1);
   return elAngle;
}

/**
   @brief Gets the start datetime of a dataset.
   @param dataset Dataset name.
   @return The start datetime as a string or std::nullopt if it does not exist.
*/ 
optional<string> HoofHomogenizer::_getStartDatetime(const string& dataset)
{
   // the startdate and starttime attributes are in the what group of the dataset
   string datetimeGroup = dataset + "/what";
   optional<string> startdate = _getAtt<string>(datetimeGroup, "startdate");
   optional<string> starttime = _getAtt<string>(datetimeGroup, "starttime");

   optional<string> startdatetime = std::nullopt;
   if(startdate && starttime)
      startdatetime = startdate.value() + starttime.value();
   return startdatetime;
}

/** 
   @brief Gets the HOOF task name of a quality group.
   @param groups The group hierarchy of the quality group. 
   @return The task as a string recognized by HOOF or std::nullopt if it does not exist. 
*/
optional<string> HoofHomogenizer::_getHoofTaskName(const string& quality)
{
   // the task is in the how group in the quality group
   string taskGroup = quality + "/how";
   optional<string> origTask = _getAtt<string>(taskGroup, "task");
   optional<string> task = std::nullopt;

   // set the task keywords according to the task name
   if(origTask)
   {
      if(origTask.value().find("ropo") != string::npos)
         task = "ROPO";
      else if(origTask.value().find("beamblockage") != string::npos)
         task = "BLOCK";
      else if(origTask.value().find("satfilter") != string::npos)
         task = "SAT";
      else if(origTask.value().find("qi_total") != string::npos)
         task = "TOTAL"; 
   }
   return task;
}

/**
   @brief Fills a 2D vector with dataset values from the data group in the homogenized file
      recalculated to double values.
   @param vec The vector to fill.
   @param group Group of the dataset.
   @param name Name of the dataset.
*/ 
void HoofHomogenizer::_fillHomDataDataset(vector2D<double>& vec, const string& group, const string& name)
{
   // get the dataset from the file
   optional<vector2D<unsigned char>> dataset = _outFile.getDataset(group, name);
   if(dataset)
   {
      vector<vector<unsigned char>> d = dataset.value();
  
      // get the needed metadata from the same group to recalculate dataset to double values
      optional<double> gain = _getHomAtt<double>(group + "/what", "gain");
      optional<double> offset = _getHomAtt<double>(group + "/what", "offset");
      optional<double> nodata = _getHomAtt<double>(group + "/what", "nodata");
      optional<double> undetect = _getHomAtt<double>(group + "/what", "undetect");

      // fill the vector and replace nodata and undetect values with NaNs
      if(gain && offset && nodata && undetect)
      {
         double g = gain.value();
         double o = offset.value();
         double nd = g * (double)nodata.value() + o;
         double un = g * (double)undetect.value() + o;
         int naz = dataset.value().size();
         int nr = dataset.value()[0].size();
         for(int i=0; i<naz; i++)
         {
            for(int j=0; j<nr; j++)            
               vec[i][j] = g * (double)d[i][j] + o;
         }
         HoofAux::replace<double>(vec, nd, dNaN);
         HoofAux::replace<double>(vec, un, dNaN);
      }
   }
}

/**
   @brief Fills a 2D vector with a dataset values from a quality group in the homogenized file 
      recalculated to double values.
   @param vec The vector to fill.
   @param group Group of the dataset.
   @param name Name of the dataset.
   @param nodata The value to use for no data.
*/ 
void HoofHomogenizer::_fillHomQualDataset(vector2D<double>& vec, const string& group, const string& name,
   double nodata)
{
   // get the dataset from the file
   optional<vector2D<unsigned char>> dataset = _outFile.getDataset(group, name);
   if(dataset)
   {
      // get the needed metadata from the same group to recalculate dataset to double values
      optional<double> gain = _getHomAtt<double>(group + "/what", "gain");
      optional<double> offset = _getHomAtt<double>(group + "/what", "offset");
      vector2D<unsigned char> d = dataset.value();

      // fill the vector an replace nodata values with NaNs
      if(gain && offset)
      {
         double g = gain.value();
         double o = offset.value();
         double nd = g * nodata + o;
         int naz = dataset.value().size();
         int nr = dataset.value()[0].size();
         for(int i=0; i<naz; i++)
         {
            for(int j=0; j<nr; j++)            
               vec[i][j] = g * (double)d[i][j] + o;
         }
         HoofAux::replace<double>(vec, nd, dNaN);
      }
   }
}

/**
   @brief Determines if a data or a quality group is of a particular homogenization quantity type. 
   @param type Type of the quantity ("DBZ", "TH", "VRAD" or "QUALITY").
   @param dataset Dataset group name of the quantity.
   @param data Data group name of the quantity.
   @return True if it is the requested quantity, false if not.
*/
bool HoofHomogenizer::_isQtyType(const string& type, const string& dataset, const string& data) const
{
   // get the correct quantity names according to the requested quantity
   vector<string> qtyNames;
   if(type == "DBZ")
      qtyNames = HoofSettings::dbzNames;
   else if(type == "TH")
      qtyNames = HoofSettings::thNames;
   else if(type == "VRAD")
      qtyNames = HoofSettings::vradNames;

   // check the quantity attribute in what group
   string qtyGroup = dataset + "/" + data + "/what";
   optional<string> qty = _inFile.getAtt<string>(qtyGroup, "quantity");
   if(qty)
   {
      if(HoofAux::find(qty.value(), qtyNames)) 
         return true;
   }
   return false;
}

/** 
   @brief Finds homogenization quantities with an elevation angle, datetime and optionally task
          or the new dataset group in a vector.
   @param qtys The vector of quantities to search. 
   @param elAngle The elevation angle to search for.
   @param datetime The datetime to search for.
   @param task Optionally the task to search for.
   @param newDataset Optionally the new dataset group name to search for.
   @return The vector of found quantities or std::nullopt if not found.
*/
optional<vector<HoofHomQty>> HoofHomogenizer::_findQtys(const vector<HoofHomQty>& qtys,
   double elAngle, const string& datetime, const string& task, const string& newDataset) const
{
   vector<HoofHomQty> foundQtys;
   // if new dataset group name is present, we only search for it
   if(newDataset != "")
   {
      for(int i=0; i<qtys.size(); i++)
      {
         if(newDataset == qtys[i].newDataset)
            foundQtys.push_back(qtys[i]);
      }
   }
   else
   {
      // if task is present we search for the angle, datetime and task name
      if(task != "")
      {
         for(int i=0; i<qtys.size(); i++)
         {
            if(HoofAux::eqDbl(elAngle, qtys[i].elAngle) && datetime == qtys[i].datetime &&
               task == qtys[i].task)
               foundQtys.push_back(qtys[i]);
         }
      }
      // otherwise, we search for angle and datetime
      else
      {
         for(int i=0; i<qtys.size(); i++)
         {
            if(HoofAux::eqDbl(elAngle, qtys[i].elAngle) && datetime == qtys[i].datetime)
               foundQtys.push_back(qtys[i]);
         }
      }
   }

   // return found quantities or std::nullopt
   optional<vector<HoofHomQty>> result = std::nullopt;
   if(foundQtys.size() > 0)
      result = foundQtys;
   return result;
}

/**
 * @brief Gets all homogenization quantities from the file.
 * @param dbzs Found DBZ quantities.
 * @param ths Found TH quantities.
 * @param vrads Found VRAD quantities.
 * @param quals Found QUALITYn quantities.
 */
void HoofHomogenizer::_getQtys(vector<HoofHomQty>& dbzs, vector<HoofHomQty>& ths, vector<HoofHomQty>& vrads,
   std::vector<HoofHomQty>& quals)
{
   vector<string> datasets = _inFile.getDatasets();
   for(int i=0; i<datasets.size(); i++)
   {
      string dataset = datasets[i];
      
      // get elevation angle and start datetime and check that they exist, otherwise skip the dataset
      optional<double> elAngle = _getRoundedElAngle(dataset);
      optional<string> startDatetime = _getStartDatetime(dataset);
      if(!elAngle || !startDatetime)
      {
         warning("no date or elevation angle in dataset " + dataset + ", skipping it");
         continue;
      }

      // create DBZ, TH and VRAD homogenization quantities from data groups in the current dataset
      vector<string> dataGroups = _inFile.getDatas(dataset, "data");
      for(int j=0; j<dataGroups.size(); j++)
      {
         string data = dataGroups[j];
         if(_isQtyType("DBZ", dataset, data))
            dbzs.push_back(HoofHomQty("DBZ", elAngle.value(), startDatetime.value(), "", dataset, data));
         else if(_isQtyType("TH", dataset, data))
            ths.push_back(HoofHomQty("TH", elAngle.value(), startDatetime.value(), "", dataset, data));        
         else if(_isQtyType("VRAD", dataset, data))
            vrads.push_back(HoofHomQty("VRAD", elAngle.value(), startDatetime.value(), "", dataset, data));
      }

      // create QUALITYn homogenization quantities from quality groups in the current dataset
      int qualNum = 0;
      vector<string> qualGroups = _inFile.getDatas(dataset, "quality");
      for(int j=0; j<qualGroups.size(); j++)
      {
         optional<string> task = _getHoofTaskName(dataset + "/" + qualGroups[j]);
         if(task)
         {   
            if(HoofAux::find<string>(task.value(), HoofSettings::dbzQualNames))
            {
               qualNum++;
               quals.push_back(HoofHomQty("QUALITY" + HoofAux::string<int>(qualNum),
                  elAngle.value(), startDatetime.value(), task.value(), dataset, qualGroups[j]));
            }
         }
      }
   }
}

/**
   @brief Sorts the TH quantities into DBZ datasets.
   
   Puts the TH quantities in new DBZ datasets if they have the same elevation angle and start datetime.
   If there are more than one such DBZ datasets found, only take the one that has the same original dataset
   group. If no TH quantity is found with corresponding DBZ quantity, skip it.
   
   @param ths TH quantities to sort.
   @param dbzs DBZ quantities to sort to. 
   @param newThs The sorted TH quantities to be filled.
*/
void HoofHomogenizer::_sortThs(const vector<HoofHomQty>& ths, const vector<HoofHomQty>& dbzs,
   vector<HoofHomQty>& newThs)
{
   for(int i=0; i<ths.size(); i++)
   {
      HoofHomQty th = ths[i];
      optional<vector<HoofHomQty>> dbz = _findQtys(dbzs, th.elAngle, th.datetime);
      bool thFound = false;
      if(dbz)
      {
         vector<HoofHomQty> d = dbz.value();
         if(d.size() == 1)
         {
            th.newDataset = d[0].newDataset;
            th.newData = "data2";
            newThs.push_back(th);
            thFound = true;
         }
         else if(d.size() > 1)
         {
            warning("More than one DBZ quantity matches the TH quantity in " + th.oldDataset + "/" +
               th.oldData);
            for(int j=0; j<d.size(); j++)
            {
               if(th.oldDataset == d[j].oldDataset)
               {
                  th.newDataset = d[j].newDataset;
                  th.newData = "data2";
                  newThs.push_back(th);
                  thFound = true;
                  break;
               }
            }   
         }
      }
      if(!thFound)
         warning("TH quantity in " + th.oldDataset + "/" + th.oldData +
            " has no matching DBZ group, omitting it");
   }
}

/**
   @brief Checks if each DBZ quantity has a corresponding TH quantity and check that
   dimensions of DBZ and TH quantities match, skip both if not.   
   @param dbzs DBZ quantities to check.
   @param ths TH quantities to check for. 
   @param newDbzs The DBZ quantities that have a corresponding TH quantity to be filled.
   @param newDbzs The TH quantities that orrespond to a DBZ quantity to be filled.
*/
void HoofHomogenizer::_checkDbzs(const vector<HoofHomQty>& dbzs, const vector<HoofHomQty>& ths,
   vector<HoofHomQty>& newDbzs, vector<HoofHomQty>& newThs)
{
   for(int i=0; i<dbzs.size(); i++)
   {
      HoofHomQty dbz = dbzs[i];
      optional<vector<HoofHomQty>> foundThs = _findQtys(ths, -999.9, "", "", dbz.newDataset);
      if(foundThs)
      {
         HoofHomQty th = foundThs.value()[0];
         optional<int> nazDbz = _inFile.getAtt<int>(dbz.oldDataset + "/where", "nrays");
         optional<int> nrDbz = _inFile.getAtt<int>(dbz.oldDataset + "/where", "nbins");
         optional<int> nazTh = _inFile.getAtt<int>(th.oldDataset + "/where", "nrays");
         optional<int> nrTh = _inFile.getAtt<int>(th.oldDataset + "/where", "nbins");
         if(nazDbz && nrDbz && nazTh && nrTh)
         {
            if(nazDbz.value() == nazTh.value() && nrDbz.value() == nrTh.value())
            {
               newThs.push_back(th);
               newDbzs.push_back(dbz);
            }
            else
            {
               warning("DBZ quantity in " + dbz.oldDataset + "/" + dbz.oldData +
                  " has a matching TH quantity, but dimensions are not the same, omitting both");               
            }
         }
      }
      else
         warning("DBZ quantity in " + dbz.oldDataset + "/" + dbz.oldData +
                 " has no corresponding TH group, omitting it");
   }
} 
/**
   @brief Sorts QUALITYn quantities into DBZ or VRAD datasets.
   
   Puts the QUANTITYn quantities in new DBZ or VRAD  datasets if they have the same elevation angle
   and start datetime and skips it if no such dataset is found. Assumes that only one quantity matches the
   QUALITYn group.

   @param quals QUALITYn quantities to sort.
   @param dbzs DBZ quantities to sort to. 
   @param vrads VRAD quantities to sort to. 
   @param newQuals The QUALITYn quantities that have a corresponding DBZ or VRAD dataset.
*/
void HoofHomogenizer::_sortQuals(const vector<HoofHomQty>& quals, const vector<HoofHomQty>& dbzs,
   const vector<HoofHomQty>& vrads, vector<HoofHomQty>& newQuals)
{
   for(int i=0; i<quals.size(); i++)
   {
      HoofHomQty qual = quals[i];
      optional<vector<HoofHomQty>> dbz = _findQtys(dbzs, qual.elAngle, qual.datetime);
      optional<vector<HoofHomQty>> vrad = _findQtys(vrads, qual.elAngle, qual.datetime);
      bool qualFound = false;
      if(dbz)
      {
         qual.newDataset = dbz.value()[0].newDataset;
         qual.newData = "quality" + qual.name.substr(qual.name.find("quality")+8);
         newQuals.push_back(qual);
         qualFound = true;
      }
      if(vrad)
      {
         qual.newDataset = vrad.value()[0].newDataset;
         qual.newData = "quality" + qual.name.substr(qual.name.find("quality")+8);
         newQuals.push_back(qual);
         qualFound = true;
      }
      if(!qualFound)
         warning("QUALITY quantity in " + qual.oldDataset + "/" + qual.oldData +
                  " has no matching DBZ or VRAD group, omitting it");
   }
}

/**
   @brief Checks if a quantity has the required quality groups.
   @param quals Vector of quality homogenization quantities.
   @param elAngle Elevation angle of the quantity to check.
   @param datetime Datetime of the quantity to check.
   @param reqNames Vector of required quality groups.
   @return True if it has the groups, false otherwise.
*/
bool HoofHomogenizer::_hasReqQualGroups(const vector<HoofHomQty>& quals, const double elAngle,
    const string& datetime, const vector<string>& reqNames) const
{ 
   // get tasks of the quality groups in found quantities
   optional<vector<HoofHomQty>> currQuals = _findQtys(quals, elAngle, datetime);
   vector<string> currQualTasks;
   if(currQuals)
   {
      for(int j=0; j<currQuals.value().size(); j++)
         currQualTasks.push_back(currQuals.value()[j].task);
   }

   // check if tasks of the required quality groups exist in the found quality groups
   bool hasRequiredQualGroups = true;
   for(int j=0; j<reqNames.size(); j++)
   {
      if(!HoofAux::find<string>(reqNames[j], currQualTasks))
      {
         hasRequiredQualGroups = false;
         break;
      }
   }
   
   return hasRequiredQualGroups;
}

/**
   @brief Checks that each DBZ has the required quality groups otherwise omit the whole dataset.
  
   @param dbzs DBZ quantities to check.
   @param ths TH quantities corresponding to checked DBZ quantities.
   @param quals QUALITYn quantities corresponding to checked DBZ and VRAD quantities.
   @param newDbzs DBZ quantities that have required quality groups that gets filled.
   @param newThs TH corresponding to DBZ quantities that have required quality groups that gets filled.
   @param newQuals QUALITYn quantities corresponding to DBZ and VRAD quantities that have required quality
                   groups that gets filled.
*/
void HoofHomogenizer::_checkReqDbzsVrads(const vector<HoofHomQty>& dbzs, const vector<HoofHomQty>& ths,
   const vector<HoofHomQty>& quals, vector<HoofHomQty>& newDbzs, vector<HoofHomQty>& newThs,
   vector<HoofHomQty>& newQuals)
{
   for(int i=0; i<dbzs.size(); i++)
   {
      HoofHomQty dbz = dbzs[i];

      bool hasGroups = _hasReqQualGroups(quals, dbz.elAngle, dbz.datetime, HoofSettings::dbzQualNames);
      if(hasGroups)
      {
         optional<vector<HoofHomQty>> correspQuals = _findQtys(quals, -999.9, "", "", dbz.newDataset);
         optional<vector<HoofHomQty>> correspThs = _findQtys(ths, -999.9, "", "", dbz.newDataset);
         if(correspQuals && correspThs)
         {
            newDbzs.push_back(dbz);
            newThs.insert(newThs.end(), correspThs.value().begin(), correspThs.value().end());
            newQuals.insert(newQuals.end(), correspQuals.value().begin(), correspQuals.value().end());
         }
      }
      else
         warning("DBZ quantity in " + dbz.oldDataset + "/" + dbz.oldData +
                 " does not have the required quality groups, omitting dataset");
   }
}

/**
   @brief Recounts homogenization quantities so that datasets always begin with 1
   @param dbzs DBZ quantities to recount.
   @param ths TH quantities to recount.
   @param vrads VRAD quantities to recount.
   @param quals QUALITYn quantities to recount.
   @param newDbzs Recounted DBZ quantities.
   @param newThs Recounted TH quantities.
   @param newVrads Recounted VRAD quantities.
   @param newQuals Recounted QUALITYn quantities.
*/
void HoofHomogenizer::_recountQtys(const vector<HoofHomQty>& dbzs, const vector<HoofHomQty>& ths,
   const vector<HoofHomQty>& vrads, const vector<HoofHomQty>& quals, vector<HoofHomQty>& newDbzs,
   vector<HoofHomQty>& newThs, vector<HoofHomQty>& newVrads, vector<HoofHomQty>& newQuals)
{
   int fdCnt = 0;
   for(int i=0; i<dbzs.size(); i++)
   {
      fdCnt++;
      HoofHomQty dbz = dbzs[i];
      string newDataset = "dataset" + HoofAux::string<int>(fdCnt);
      optional<vector<HoofHomQty>> currThs = _findQtys(ths, -999.9, "", "", dbz.newDataset);
      optional<vector<HoofHomQty>> currQuals = _findQtys(quals, -999.9, "", "", dbz.newDataset);
      if(currThs)
      {
         currThs.value()[0].newDataset = newDataset;
         currThs.value()[0].newData = "data2";
      }
      if(currQuals)
      {
         for(int j=0; j<currQuals.value().size(); j++)
         {
            string qualNum = currQuals.value()[j].name.substr(currQuals.value()[j].name.find("quality")+8);
            currQuals.value()[j].newDataset = newDataset;
            currQuals.value()[j].newData = "quality" + qualNum;
         }
      }
      dbz.newDataset = newDataset;
      dbz.newData = "data1"; 
      newDbzs.push_back(dbz);
      if(currThs)      
         newThs.push_back(currThs.value()[0]);
      if(currQuals)
         newQuals.insert(newQuals.end(), currQuals.value().begin(), currQuals.value().end());   
   }
   for(int i=0; i<vrads.size(); i++)
   {
      fdCnt++;
      HoofHomQty vrad = vrads[i];
      string newDataset = "dataset" + HoofAux::string<int>(fdCnt);
      optional<vector<HoofHomQty>> currQuals = _findQtys(quals, -999.9, "", "", vrad.newDataset);
      if(currQuals)
      {
         for(int j=0; j<currQuals.value().size(); j++)
         {
            string qualNum = currQuals.value()[j].name.substr(currQuals.value()[j].name.find("quality")+8);
            currQuals.value()[j].newDataset = newDataset;
            currQuals.value()[j].newData = "quality" + qualNum;
         }
      }
      vrad.newDataset = newDataset;
      vrad.newData = "data1";
      newVrads.push_back(vrad);
      if(currQuals)
         newQuals.insert(newQuals.end(), currQuals.value().begin(), currQuals.value().end());         
   }  
}

/**
   @brief Checks and writes attributes from metadata groups of a group type in a homogenization quantity
      either from namelist or input file and writes them to the output file.
   @param groupType "root", "dataset", "data" or "quality".
   @param qty The homogenization quantity for which to check and write the attributes.
*/
void HoofHomogenizer::_checkAndWriteQtyMetadataGroups(const string& groupType, const HoofHomQty& qty)
{
   // get the namelist metadata groups for the group type
   vector<string> metaGroups = _getNamelistMetadataGroups(groupType); 

   for(int i=0; i<metaGroups.size(); i++)
   {
      string metaGroup = metaGroups[i];

      // get the attributes for the current namelist metadata group
      vector<HoofNamAtt> atts = _getNamelistGroupAtts(metaGroup);

      // get the old and new quantity groups for the group type
      string oldQtyGroup;
      string newQtyGroup;
      string meta = HoofAux::split(metaGroup, "/", " ").back();
      if(groupType == "root")
      {
         oldQtyGroup = meta;
         newQtyGroup = meta;
      }  
      if(groupType == "dataset")
      {
         oldQtyGroup = qty.oldDataset + "/" + meta;
         newQtyGroup = qty.newDataset + "/" + meta;
      }
      if(groupType == "data" || groupType == "quality")
      {
         oldQtyGroup = qty.oldDataset + "/" + qty.oldData + "/" + meta;
         newQtyGroup = qty.newDataset + "/" + qty.newData + "/" + meta;
      }
      if(groupType == "quality" && qty.name != "VRAD")
      {
         oldQtyGroup = qty.oldDataset + "/" + qty.oldData + "/" + meta;
         newQtyGroup = qty.newDataset + "/" + qty.newData + "/" + meta;
      }      

      // check and write all attributes in group to the file
      for(int j=0; j<atts.size(); j++)
      {         
         HoofNamAtt att = atts[j];

         // handle string attributes (quantity has to be dealt with explicitly)
         if(att.type == "S")
         {
            optional<string> sValue = _getAtt<string>(oldQtyGroup, att.name);
            if(sValue)
            {
               if(att.name != "quantity")
                  _outFile.writeAtt<string>(newQtyGroup, att.name, sValue.value());
               else
                  _outFile.writeAtt<string>(newQtyGroup, att.name, qty.name);
            }
         }
         // handle integer attributes
         else if(att.type == "I")
         {
            optional<int> iValue = _getAtt<int>(oldQtyGroup, att.name);
            if(iValue)
               _outFile.writeAtt<int>(newQtyGroup, att.name, iValue.value());
         }
         // handle float attributes
         else if(att.type == "F")
         {
            optional<double> dValue = _getAtt<double>(oldQtyGroup, att.name);
            if(dValue)
               _outFile.writeAtt<double>(newQtyGroup, att.name, dValue.value());
         }
      }
   }
}

/**
   @brief Finds all homogenization quantities in the input file satisfying the namelist criteria and sorts
      them in a homogenized order.
*/
void HoofHomogenizer::sort()
{
   // clear the _qtys vector if it is somehow already filled
   _qtys.clear();

   // get all homogenization quantities from the file
   vector<HoofHomQty> dbzs;
   vector<HoofHomQty> ths;
   vector<HoofHomQty> vrads;
   vector<HoofHomQty> quals;
   _getQtys(dbzs, ths, vrads, quals);

   // sort the dbz and vrad quantities by start datetime
   std::sort(dbzs.begin(), dbzs.end());
   std::sort(vrads.begin(), vrads.end());

   // determine the new groups for DBZ and VRAD quantities
   int dCnt = 0;
   for(int i=0; i<dbzs.size(); i++)
   {
      dCnt++;
      dbzs[i].newDataset = "dataset" + HoofAux::string<int>(dCnt);
      dbzs[i].newData = "data1";
   }
   for(int i=0; i<vrads.size(); i++)
   {
      dCnt++;
      vrads[i].newDataset = "dataset" + HoofAux::string<int>(dCnt);
      vrads[i].newData = "data1";
   }

   // sort the TH quantities into DBZ datasets, skip the TH quantity if no corresponding DBZ dataset is found
   vector<HoofHomQty> sortedThs;
   _sortThs(ths, dbzs, sortedThs);

   // check that each DBZ quantity has a corresponding TH quantity and that dimensions
   // of DBZ and TH quantities match and skip it if not
   vector<HoofHomQty> thCheckedDbzs;
   vector<HoofHomQty> thCheckedThs;
   _checkDbzs(dbzs, sortedThs, thCheckedDbzs, thCheckedThs);

   // sort QUALITYn quantities into DBZ or VRAD datasets, skip it if it has no corresponding quantity
   vector<HoofHomQty> sortedQuals;
   _sortQuals(quals, thCheckedDbzs, vrads, sortedQuals);

   // check that each DBZ and VRAD quantity has the required quality groups otherwise omit the whole dataset
   vector<HoofHomQty> reqThCheckedDbzs;
   vector<HoofHomQty> reqThCheckedThs;
   vector<HoofHomQty> reqCheckedQuals;
   _checkReqDbzsVrads(thCheckedDbzs, thCheckedThs, sortedQuals, reqThCheckedDbzs, reqThCheckedThs,
      reqCheckedQuals);

   // after all checks, recount the datasets and correct the newGroups, so they always start with dataset 1
   vector<HoofHomQty> finalDbzs;
   vector<HoofHomQty> finalThs;
   vector<HoofHomQty> finalVrads;
   vector<HoofHomQty> finalQuals;
   _recountQtys(reqThCheckedDbzs, reqThCheckedThs, vrads, reqCheckedQuals, finalDbzs, finalThs, finalVrads,
      finalQuals); 

   // save all quantity lists to the quantities list
   _qtys.insert(_qtys.end(), finalDbzs.begin(), finalDbzs.end());
   _qtys.insert(_qtys.end(), finalThs.begin(), finalThs.end());
   _qtys.insert(_qtys.end(), finalQuals.begin(), finalQuals.end());
   _qtys.insert(_qtys.end(), finalVrads.begin(), finalVrads.end());
}

/**
   @brief Checks if attributes required in the namelist have values either in the namelist or in the
      input file and then writes them to the output file.
*/
void HoofHomogenizer::checkAndWrite()
{
   // handle Conventions attribute
   optional<string> conventions = _inFile.getAtt<string>("/", "Conventions");
   if(conventions)
      _outFile.writeAtt<string>("/", "Conventions", conventions.value());
   else  
      error("Conventions attribute not found");

   // handle the root group metadata attributes
   HoofHomQty dummy;
   _checkAndWriteQtyMetadataGroups("root", dummy);

   // check if there are any quantities to write
   if(_qtys.size() == 0)
      error("no quantities to write to output file");

   // loop on quantities
   for(int i=0; i<_qtys.size(); i++)
   {
      HoofHomQty qty = _qtys[i];

      // handle the dataset group metadata attributes
      // (check only DBZ and VRAD, since they have unique datasets)
      if(qty.name == "DBZ" || qty.name == "VRAD")
         _checkAndWriteQtyMetadataGroups("dataset", qty);

      // handle the data group metadata attributes and copy dataset
      if(qty.oldData.find("data") != string::npos)
      {
         _checkAndWriteQtyMetadataGroups("data", qty);
         _inFile.copyDataset(_outFile, qty.oldDataset + "/" + qty.oldData + "/data",
            qty.newDataset + "/" + qty.newData + "/data");
      }

      // handle the quality group metadata attributes and copy dataset
      if(qty.oldData.find("quality") != string::npos)
      {
         _checkAndWriteQtyMetadataGroups("quality", qty);      
         _inFile.copyDataset(_outFile, qty.oldDataset + "/" + qty.oldData + "/data",
            qty.newDataset + "/" + qty.newData + "/data");
      }
   }
   _outFile.flush();  
}

/**
   @brief Stores homogenized data to a HoofData object for further use.
*/
void HoofHomogenizer::storeData()
{
   // get names and length of VRAD and DBZ datasets and the corresponding TOTAL QUALITY group
   for(int i=0; i<_qtys.size(); i++)
   {
      HoofHomQty qty = _qtys[i];

      // get DBZ datasets and TOTAL QUALITY groups 
      if(qty.name == "DBZ")
      {
         _data.dbz.datasets.push_back(qty.newDataset);
         if(HoofSettings::superobing)
         {
            optional<vector<HoofHomQty>> qualQtys = _findQtys(_qtys, qty.elAngle, qty.datetime, "TOTAL");
            if(qualQtys)
               _data.dbz.qualdatas.push_back(qualQtys.value()[0].newData);
            else
               _data.dbz.qualdatas.push_back("None");
         }
      }

      // get VRAD datasets
      if(qty.name == "VRAD")
      {
         _data.vrad.datasets.push_back(qty.newDataset);
         if(HoofSettings::superobing)
         {
            optional<vector<HoofHomQty>> qualQtys = _findQtys(_qtys, qty.elAngle, qty.datetime, "TOTAL");
            if(qualQtys)
               _data.vrad.qualdatas.push_back(qualQtys.value()[0].newData);
         }
      }         
   }
   _data.dbz.nel = _data.dbz.datasets.size();
   _data.vrad.nel = _data.vrad.datasets.size();

   // get radar site height
   optional<double> height = _getAtt<double>("where", "height");
   if(height)
      _data.height = height.value();

   // handle DBZ related data
   if(_data.dbz.nel > 0)
   {
      // initialize needed DBZ arrays with NaNs
      int nel = _data.dbz.nel;
      _data.dbz.naz = vector<int>(nel, iNaN);
      _data.dbz.nr = vector<int>(nel, iNaN);
      for(int i=0; i<nel; i++)
      {
         string dataset = _data.dbz.datasets[i];
         optional<int> az = _getHomAtt<int>(dataset + "/where", "nrays");
         if(az)
            _data.dbz.naz[i] = az.value();
         optional<int> rb = _getHomAtt<int>(dataset + "/where", "nbins");
         if(rb)
            _data.dbz.nr[i] = rb.value();
      }         
      int naz = *max_element(_data.dbz.naz.begin(), _data.dbz.naz.end());
      int nr = *max_element(_data.dbz.nr.begin(), _data.dbz.nr.end());
      _data.dbz.nazMax = naz;
      _data.dbz.nrMax = nr;
      _data.dbz.elangles = vector<double>(nel, dNaN);
      _data.dbz.azimuths = vector2D<double>(nel, vector<double>(naz, dNaN));
      _data.dbz.ranges = vector2D<double>(nel, vector<double>(nr, dNaN));
      _data.dbz.rstarts = vector<double>(nel, dNaN);
      _data.dbz.rscales = vector<double>(nel, dNaN);
      _data.dbz.meas = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
      _data.dbz.ths = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
      _data.dbz.quals = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));

      // fill the DBZ arrays with data from the homogenized file
      for(int i=0; i<nel; i++)
      {
         string dataset = _data.dbz.datasets[i];
         int a = _data.dbz.naz[i];
         int r = _data.dbz.nr[i];
         optional<double> elangle = _getHomAtt<double>(dataset + "/where", "elangle");
         if(elangle)
            _data.dbz.elangles[i] = elangle.value() * HoofAux::Pi / 180.0;
         HoofAux::linspace(_data.dbz.azimuths[i], 0.0, 2*HoofAux::Pi, a);
         optional<double> rstart = _getHomAtt<double>(dataset + "/where", "rstart");
         if(rstart)
            _data.dbz.rstarts[i] = rstart.value();
         optional<double> rscale = _getHomAtt<double>(dataset + "/where", "rscale");
         if(rscale)
            _data.dbz.rscales[i] = rscale.value();
         if(rscale && rstart)
            HoofAux::linspace(_data.dbz.ranges[i], rstart.value(),
               rstart.value() + rscale.value()*(double)r, r);
         _fillHomDataDataset(_data.dbz.meas[i], dataset + "/data1", "data");
         _fillHomDataDataset(_data.dbz.ths[i], dataset + "/data2", "data");
         if(HoofSettings::superobing)
         {
            optional<double> nodata = _getHomAtt<double>(dataset + "/data1/what", "nodata");
            if(nodata)
               _fillHomQualDataset(_data.dbz.quals[i], dataset + "/" + _data.dbz.qualdatas[i],
                  "data", nodata.value());
         }        
      }
   }

   // handle VRAD related data
   if(_data.vrad.nel > 0)
   {
      // initialize needed VRAD arrays with NaNs
      int nel = _data.vrad.nel;
      _data.vrad.naz = vector<int>(nel, iNaN);
      _data.vrad.nr = vector<int>(nel, iNaN);
      for(int i=0; i<nel; i++)
      {
         string dataset = _data.vrad.datasets[i];
         optional<int> az = _getHomAtt<int>(dataset + "/where", "nrays");
         if(az)
            _data.vrad.naz[i] = az.value();
         optional<int> rb = _getHomAtt<int>(dataset + "/where", "nbins");
         if(rb)
            _data.vrad.nr[i] = rb.value();
      }
      int naz = *max_element(_data.vrad.naz.begin(), _data.vrad.naz.end());
      int nr = *max_element(_data.vrad.nr.begin(), _data.vrad.nr.end());
      _data.vrad.nazMax = naz;
      _data.vrad.nrMax = nr;
      _data.vrad.elangles = vector<double>(nel, dNaN);
      _data.vrad.azimuths = vector<vector<double>>(nel, vector<double>(naz, dNaN));
      _data.vrad.ranges = vector<vector<double>>(nel, vector<double>(nr, dNaN));
      _data.vrad.rstarts = vector<double>(nel, dNaN);
      _data.vrad.rscales = vector<double>(nel, dNaN);
      _data.vrad.vnys = vector<double>(nel, dNaN);
      _data.vrad.meas = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));
      _data.vrad.zs = vector3D<double>(nel, vector2D<double>(naz, vector<double>(nr, dNaN)));         

      // fill the VRAD matrices with data from the homogenized file
      for(int i=0; i<nel; i++)
      {
         string dataset = _data.vrad.datasets[i];
         int a = _data.vrad.naz[i];
         int r = _data.vrad.nr[i];
         optional<double> elangle = _getHomAtt<double>(dataset + "/where", "elangle");
         if(elangle)
            _data.vrad.elangles[i] = elangle.value() * HoofAux::Pi / 180.0;
         HoofAux::linspace(_data.vrad.azimuths[i], 0.0, 2*HoofAux::Pi, a);
         optional<double> rstart = _getHomAtt<double>(dataset + "/where", "rstart");
         if(rstart)
            _data.vrad.rstarts[i] = rstart.value();
         optional<double> rscale = _getHomAtt<double>(dataset + "/where", "rscale");
         if(rscale)
            _data.vrad.rscales[i] = rscale.value();
         if(rscale && rstart)
            HoofAux::linspace(_data.vrad.ranges[i], rstart.value(), 
               rstart.value() + rscale.value()*(double)r, r);
         optional<double> vny = _getHomAtt<double>(dataset + "/how", "NI");
         if(vny)
            _data.vrad.vnys[i] = vny.value();
         _fillHomDataDataset(_data.vrad.meas[i], dataset + "/data1", "data");
      }
   
      // calculate heights for all vrad measurements from Equivalent Earth model
      double R = HoofAux::earthRadius;
      double K = HoofAux::eqEarthFactor;
      double KR = K*R;
      double KRsq = KR*KR;
      double KRh = KR - _data.height;
      for(int i=0; i<_data.vrad.nel; i++)
      {
         double twoKRsinA = 2*KR*sin(_data.vrad.elangles[i]);
         for(int j=0; j<_data.vrad.naz[i]; j++)
         {
            for(int k=0; k<_data.vrad.nr[i]; k++)
            {
               double r = _data.vrad.ranges[i][k];
               _data.vrad.zs[i][j][k] = sqrt(r*r + KRsq + r*twoKRsinA) - KRh;
            }
         }
      }
   }
}