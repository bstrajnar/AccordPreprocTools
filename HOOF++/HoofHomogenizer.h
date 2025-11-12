/**
   @file HoofHomogenizer.h
   @author Peter Smerkol
   @brief Contains definition of HoofHomogenizer class.
*/

#ifndef HOOFHOMOGENIZER_GUARD
#define HOOFHOMOGENIZER_GUARD

#include <string>
#include <vector>
#include <optional>
#include <HoofTypes.h>
#include <HoofWorker.h>
#include <HoofH5File.h>
#include <HoofData.h>
#include <HoofNamAtt.h>
#include <HoofHomQty.h>

/**
   @class HoofHomogenizer
   @brief Worker object that homogenizes one HDF5 file.
*/
class HoofHomogenizer : public HoofWorker
{
   private:
      // members 
      HoofH5File& _inFile;             ///< The input file.
      HoofH5File& _outFile;            ///< The output file.  
      HoofData& _data;                 ///< Object that gets filled with homogenized data for further use.
      std::vector<HoofHomQty> _qtys;   ///< A vector of sorted homogenization quantities.
   
      // gets the unique namelist metadata groups by group type
      std::vector<std::string> _getNamelistMetadataGroups(const std::string& groupType) const;
      // gets namelist attributes from a namelist group 
      std::vector<HoofNamAtt> _getNamelistGroupAtts(const std::string& group) const;
      // gets an attribute value of type T either from namelist or the input or output file
      template<typename T> std::optional<T> _getAtt(const std::string& group, const std::string& name);
      // gets the elevation angle of a dataset rounded to 1 decimal
      std::optional<double> _getRoundedElAngle(const std::string& dataset);
      // gets the start datetime of a dataset of the input file
      std::optional<std::string> _getStartDatetime(const std::string& dataset);
      // gets the HOOF task name of a quality group of the input file
      std::optional<std::string> _getHoofTaskName(const std::string& quality);   
      // determines if a data or a quality group is a particular homogenization quantity
      bool _isQtyType(const std::string& name, const std::string& dataset, const std::string& data) const;
      // finds homogenization quantities with a date, elevation angle and optionally task and the new
      // dataset group in a vector
      std::optional<std::vector<HoofHomQty>> _findQtys(
         const std::vector<HoofHomQty>& qtys, double elAngle, const std::string& datetime,
         const std::string& task="", const std::string& newDataset="") const;
      // gets all homogenization quantities from the file
      void _getQtys(std::vector<HoofHomQty>& dbzs, std::vector<HoofHomQty>& ths,
         std::vector<HoofHomQty>& vrads, std::vector<HoofHomQty>& quals);
      // sorts the TH quantities into DBZ datasets
      void _sortThs(const std::vector<HoofHomQty>& ths, const std::vector<HoofHomQty>& dbzs,
         std::vector<HoofHomQty>& newThs);
      // checks if all DBZ quantities have a corresponding TH quantity
      void _checkDbzs(const std::vector<HoofHomQty>& dbzs, const std::vector<HoofHomQty>& ths,
         std::vector<HoofHomQty>& newDbzs, std::vector<HoofHomQty>& newThs);
      // sorts QUALITYn quantities into DBZ or VRAD datasets
      void _sortQuals(const std::vector<HoofHomQty>& quals, const std::vector<HoofHomQty>& dbzs,
         const std::vector<HoofHomQty>& vrads, std::vector<HoofHomQty>& newQuals);
      // checks if a quantity has the required quality groups
      bool _hasReqQualGroups(const std::vector<HoofHomQty>& qtys, const double elAngle,
         const std::string& datetime, const std::vector<std::string>& reqNames) const;
      // checks if each DBZ and VRAD quantity has the required quality groups
      void _checkReqDbzsVrads(const std::vector<HoofHomQty>& dbzs, const std::vector<HoofHomQty>& ths,
         const std::vector<HoofHomQty>& quals, std::vector<HoofHomQty>& newDbzs,
         std::vector<HoofHomQty>& newThs, std::vector<HoofHomQty>& newQuals);
      // recounts homogenization quantities so that datasets begin with 1
      void _recountQtys(const std::vector<HoofHomQty>& dbzs, const std::vector<HoofHomQty>& ths,
         const std::vector<HoofHomQty>& vrads, const std::vector<HoofHomQty>& quals,
         std::vector<HoofHomQty>& newDbzs, std::vector<HoofHomQty>& newThs,
         std::vector<HoofHomQty>& newVrads, std::vector<HoofHomQty>& newQuals);
      // checks and writes attributes from metadata groups of a group type in a homogenization quantity
      // either from namelist or input file to the output file
      void _checkAndWriteQtyMetadataGroups(const std::string& groupType, const HoofHomQty& qty);
      // fills a 2D vector with dataset values from a data group of the homogenized file,
      // recalculated to double values
      void _fillHomDataDataset(std::vector<std::vector<double>>& vec, const std::string& group,
         const std::string& name);       
      // fills a 2D vector with dataset values from a quality group of the homogenized file,
      // recalculated to double values
      void _fillHomQualDataset(hoof::vector2D<double>& vec, const std::string& group,
         const std::string& name, const double nodata); 
      // gets an attribute value of type T from the homogenized file
      template<typename T> std::optional<T> _getHomAtt(const std::string& group, const std::string& name);
         
   public:  
      // constructor
      HoofHomogenizer(HoofH5File& inFile, HoofH5File& outFile, HoofData& data);
      // finds all quantities in the input file satisfying the namelist criteria and sorts
      // them in a homogenized order.
      void sort();      
      // checks if groups and attributes required by the namelist exist in the input file and writes them to
      // the output file
      void checkAndWrite();
      // stores homogenized data to a HoofData object for further use
      void storeData();
};

#endif // HOOFHOMOGENIZER_GUARD