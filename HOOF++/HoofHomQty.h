/**
   @file HoofHomQty.h
   @author Peter Smerkol
   @brief Contains definition of HoofHomQty class.
*/

#ifndef HOOFHOMQTY_GUARD
#define HOOFHOMQTY_GUARD

#include <string>
#include <vector>

/**
   @class HoofHomQty
   @brief Class that represents one homogenization quantity, which is a quantity that is represented by
      a /dataset/data/ or a /dataset/quality/ group in the HDF5 file. Maps between the groups before and
      after homogenization.
*/
class HoofHomQty
{
   public:
      // members
      std::string name;           ///< Name of the quantity (DBZ, TH; VRAD or QUALITYn).
      double elAngle;             ///< Elevation angle of the dataset.
      std::string datetime;       ///< Start datetime of the dataset.
      std::string task;           ///< Shortened task name deduced of the quality group.
      std::string oldDataset;     ///< Dataset before homogenization.
      std::string oldData;        ///< Data group before homogenization.
      std::string newDataset;     ///< Dataset group after homogenization.
      std::string newData;        ///< Data Group after homogenization.

      // default constructor
      HoofHomQty();
      // constructor
      HoofHomQty(const std::string& _name, double _elAngle, const std::string& _datetime,
         const std::string& _task, const std::string& oldDataset, const std::string& oldData);
      // less operator used for sorting by datetime
      bool operator < (const HoofHomQty& qty) const;
};

#endif // HOOFHOMQTY_GUARD