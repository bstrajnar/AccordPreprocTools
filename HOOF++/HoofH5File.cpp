/**
   @file HoofH5File.cpp
   @author Peter Smerkol
   @brief Contains the HoofH5File class implementation.
*/

#include <string>
#include <vector>
#include <optional>
#include <type_traits>
#include <cstring>
#include <H5Cpp.h>
#include <HoofTypes.h>
#include <HoofAux.h>
#include <HoofH5File.h>

using std::string;
using std::vector;
using std::optional;
using std::is_same_v;
using namespace H5;
using namespace hoof;

/**
   @brief Default constructor.
*/
HoofH5File::HoofH5File() {}

/**
   @brief Constructor, opens a HDF5 file for reading or writing.
   @param filePath Path of the file to open.
   @param access "read" or "write".
*/
HoofH5File::HoofH5File(const string& filePath, const string& access)
{
   if(access == "read")
      _file = H5File(filePath, H5F_ACC_RDONLY);
   if(access == "write")
      _file = H5File(filePath, H5F_ACC_TRUNC);
}

/**
   @brief Gets all dataset names from the file.
   @return A vector of dataset names.
*/
vector<string> HoofH5File::getDatasets() const
{
   vector<string> datasets;
   for(hsize_t i=0; i<_file.getNumObjs(); i++)
   {
      string name = _file.getObjnameByIdx(i);
      if(name.find("dataset") != string::npos)
         datasets.push_back(name);
   }
   return datasets;
}

/**
   @brief Gets all data or quality groups in a dataset.
   @param dataset The dataset to get groups from.
   @param groupType Type of the data group ("data" or "quality").
   @return A vector of found data group names. 
*/
vector<string> HoofH5File::getDatas(const string& dataset, const string& groupType) const
{
   vector<string> datas;
   Group datasetGroup = _file.openGroup(dataset);
   for(int i=0; i<datasetGroup.getNumObjs(); i++)
   {
      string name = datasetGroup.getObjnameByIdx(i);
      if(name.find(groupType) != string::npos)
         datas.push_back(name);      
   }
   datasetGroup.close();
   return datas;
}

/**
   @brief Gets an attribute of type T.
   @param group The group of the attribute.
   @param name The name of the attribute.
   @return The found attribute or std::nullopt if group or attribute not found.
*/
template<typename T> optional<T> HoofH5File::getAtt(const string& group, const string& name) const
{
   optional<T> value = std::nullopt;
   if(_file.exists(group))
   {
      Group g = _file.openGroup(group);
      htri_t attStatus = H5Aexists(g.getId(), name.c_str());
      if(attStatus > 0)
      {
         Attribute att = g.openAttribute(name);

         // handle string attributes
         if constexpr (is_same_v<T, string>)
         {
            StrType strType = att.getStrType();
            size_t len = strType.getSize();
            char val[len+1];
            val[len] = '\0';
            att.read(strType, val);
            value = string(val);
            strType.close();
         }
         // handle double and int attributes
         else
         {
            T val;
            att.read(HDF5Type<T>::type(), &val);
            value = val;
         }
         att.close();
      }
      g.close();
   }

   return value;
}
template optional<string> HoofH5File::getAtt<string>(const string& group, const string& name) const;
template optional<double> HoofH5File::getAtt<double>(const string& group, const string& name) const;
template optional<int> HoofH5File::getAtt<int>(const string& group, const string& name) const;

/**
   @brief Creates or overwrites an attribute of type T.
   @param group The group to write to.
   @param name Attribute name.
   @param value Attribute value.
*/
template<typename T> void HoofH5File::writeAtt(const string& group, const string& name,
   const T& value) const
{
   // split groups into subgroups and create the hierarchy if it does not exist
   vector<string> groups = HoofAux::split(group, "/", " ");
   Group currGroup = _file.openGroup("/");
   for(int i=0; i<groups.size(); i++)
   {      
      if(!currGroup.exists(groups[i]))
         currGroup = currGroup.createGroup(groups[i]);
      else
         currGroup = currGroup.openGroup(groups[i]);
   }
   currGroup.close();
   
   // if attribute exists, overwrite its value, otherwise create it
   Group g = _file.openGroup(group);
   DataSpace attSpace(H5S_SCALAR);
   DataType attType;
   if constexpr(is_same_v<T,string>)
      attType = H5::StrType(H5::PredType::C_S1, H5T_VARIABLE);
   else
      attType = HDF5Type<T>::type();
   Attribute att;
   if(H5Aexists(g.getId(), name.c_str()))
      att = g.openAttribute(name);
   else
      att = g.createAttribute(name, attType, attSpace);
   att.write(attType, &value);
   att.close();

   // close to release memory
   att.close();
   attType.close();
   attSpace.close();
   g.close();
}
template void HoofH5File::writeAtt<string>(const string& group, const string& name,
   const string& value) const;
template void HoofH5File::writeAtt<double>(const string& group, const string& name,
   const double& value) const;
template void HoofH5File::writeAtt<int>(const string& group, const string& name,
   const int& value) const;

/**
   @brief Copies a dataset from this file to another file.
   @param outFile The file to copy to.
   @param oldGroup The dataset group in the old file.
   @param newGroup The dataset group in the new file.
*/
void HoofH5File::copyDataset(HoofH5File& outFile, const std::string& oldGroup,
   const std::string& newGroup) const
{
   H5Ocopy(_file.getId(), oldGroup.c_str(), outFile._file.getId(), newGroup.c_str(),
      H5P_DEFAULT, H5P_DEFAULT);
}

/**
   @brief Gets a dataset.
   @param group The dataset group.
   @param name The dataset name.
   @return The dataset, or std::nullopt if not found.
*/
optional<vector2D<unsigned char>> HoofH5File::getDataset(const string& group, const string& name) const
{
   optional<vector2D<unsigned char>> dataset = std::nullopt;

   if(_file.exists(group))
   {
      Group g = _file.openGroup(group);
      htri_t datasetStatus = H5Lexists(g.getId(), name.c_str(), H5P_DEFAULT);
      if(datasetStatus > 0)
      {
         DataSet d = g.openDataSet(name);
         DataSpace space = d.getSpace();
         int nDims = space.getSimpleExtentNdims();
         hsize_t dims[nDims];
         space.getSimpleExtentDims(dims);
         unsigned char val[dims[0]*dims[1]];
         d.read(val, PredType::NATIVE_UINT8);
         vector2D<unsigned char> values(dims[0], vector<unsigned char>(dims[1], 0));
         for(int i=0; i<dims[0]; i++)
         {
            for(int j=0; j<dims[1]; j++)
               values[i][j] = val[i*dims[1] + j];
         }
         dataset = values;
         space.close();
         d.close();         
      }
      g.close();
   }
   
   return dataset;
}

/**
   @brief Creates or replaces a dataset.
   @param group The dataset group.
   @param name The dataset name.
   @param data The data array to replace.
*/
void HoofH5File::writeDataset(const string& group, const string& name, const vector2D<unsigned char>& data)
{
   Group g = _file.openGroup(group);

   if(H5Lexists(g.getId(), name.c_str(), H5P_DEFAULT))
      H5Ldelete(g.getId(), name.c_str(), H5P_DEFAULT);

   hsize_t rows = data.size();
   hsize_t cols = data[0].size();
   hsize_t dims[2] = {rows, cols};
   DataSpace space(2, dims);
   vector<unsigned char> data1D(rows*cols);
   for(int i=0; i<rows; i++)
   {
      for(int j=0; j<cols; j++)
         data1D[i*cols + j] = data[i][j];
   }
   DataSet d = g.createDataSet(name, PredType::NATIVE_UINT8, space);
   d.write(data1D.data(), PredType::NATIVE_UINT8);
   d.close();
   space.close();
   g.close();
}

/**
   @brief Flushes the file buffer to file.
*/
void HoofH5File::flush()
{
   _file.flush(H5F_scope_t::H5F_SCOPE_GLOBAL);
}

/**
   @brief Closes the H5File to free memory.
*/
void HoofH5File::close()
{
   _file.close();
}