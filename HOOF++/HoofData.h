/**
   @file HoofData.h
   @author Peter Smerkol
   @brief Contains definition of HoofData struct.
*/

#ifndef HOOFDATA_GUARD
#define HOOFDATA_GUARD

#include <string>
#include <vector>
#include <HoofTypes.h>
#include <HoofMeasurement.h>

/**
   @struct HoofData
   @brief Struct that holds all data that needs to be transferred between worker objects.
*/
struct HoofData
{
   // members
   std::string site;                   ///< Radar site taken from the OPERA filename.
   double height;                      ///< Radar height in meters.
   HoofMeasurement dbz;                ///< All data from DBZ measurements.
   HoofMeasurement vrad;               ///< All VRAD measurements.
   std::vector<double> zStarts;        ///< Start heights of height sectors in dealiasing.
   std::vector<double> zEnds;          ///< End heights of height sectors in dealiasing.
   hoof::vector2D<hoof::Triple> zIdxs; ///< Array of (el, az, r) indexes for each height sector that are good for dealiasing.
   hoof::vector3D<double> wModels;     ///< Values of dealiasing wind model for all (el, az, r).
   hoof::vector3D<int> ns;             ///< Deailiasing Nyquist multipliers for all (el, az, r). 
   hoof::vector3D<double> dvrads;      ///< Dealiased VRAD values for all (el, az, r).
   HoofMeasurement sdbz;               ///< All data from superobed DBZ measurements.
   HoofMeasurement svrad;              ///< All data from superobed VRAD measurements. 
};

#endif // HOOFDATA_GUARD