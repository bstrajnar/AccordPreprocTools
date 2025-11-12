/**
   @file HoofH5File.h
   @author Peter Smerkol
   @brief Contains definition of HoofH5File class.
*/

#ifndef HOOFHDF5_GUARD
#define HOOFHDF5_GUARD

#include <string>
#include <vector>
#include <optional>
#include <H5Cpp.h>
#include <HoofTypes.h>

/**
   @brief Structure to return the correct HDF5 type for a type T (int and double supported).
*/
template<typename T> struct HDF5Type;
/**
   @brief Structure to return the correct HDF5 type for int.
*/
template<> struct HDF5Type<int>
{
   static H5::DataType type() {return H5::PredType::NATIVE_INT;}
};
/**
   @brief Structure to return the correct HDF5 type for double.
*/
template<> struct HDF5Type<double>
{
   static H5::DataType type() { return H5::PredType::NATIVE_DOUBLE;}   
};

/**
   @class HoofH5File
   @brief Class that wraps the HDF5 API that is needed in HOOF.
*/
class HoofH5File
{
   private:
      // members
      H5::H5File _file;   ///< The opened HDF5 file.

   public:
      // default constructor
      HoofH5File();
      // constructor
      HoofH5File(const std::string& filePath, const std::string& access);
      // gets all dataset names in the file
      std::vector<std::string> getDatasets() const;
      // gets all data or quality groups in a dataset
      std::vector<std::string> getDatas(const std::string& dataset, const std::string& groupType) const;
      // gets an attribute of type T
      template<typename T> std::optional<T> getAtt(const std::string& group, const std::string& name) const;
      // creates or replaces an attribute of type T 
      template<typename T> void writeAtt(const std::string& group, const std::string& name,
         const T& value) const;
      // copy a dataset from this file to another file
      void copyDataset(HoofH5File& outFile, const std::string& oldGroup, const std::string& newGroup) const;
      // gets a dataset
      std::optional<hoof::vector2D<unsigned char>> getDataset(const std::string& group,
         const std::string& name) const;
      // creates or replaces a dataset
      void writeDataset(const std::string& group, const std::string& name,
         const hoof::vector2D<unsigned char>& data);
      // flushes the file buffer to file
      void flush();
      // closes the H5File object to free memory
      void close();
};

#endif // HOOFH5FILE_GUARD