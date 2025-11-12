/**
   @file HoofMeasurement.h
   @author Peter Smerkol
   @brief Contains definition of HoofMeasurement struct.
*/

#ifndef HOOFMEASUREMENT_GUARD
#define HOOFMEASUREMENT_GUARD

#include <string>
#include <vector>
#include <HoofTypes.h>

/**
   @struct HoofMeasurement
   @brief Struct that holds all data for one measurement (DBZ or VRAD) from one radar volume.
*/
struct HoofMeasurement
{
   // members
   std::vector<std::string> datasets;  ///< Names of dataset groups in the HDF5 file.
   std::vector<std::string> qualdatas; ///< Names of quality groups for the TOTAL quality in the HDF5 file.
   int nel;                            ///< Number of elevations (datasets) in the HDF5 file after homogenization.
   int nazMax;                         ///< Maximum number of azimuths in elevations.
   int nrMax;                          ///< Maximum number of range bins in elevations.
   std::vector<double> elangles;       ///< Elevation angles in radians for all (el).
   std::vector<int> naz;               ///< Number of azimuths (rays) for all (el).
   hoof::vector2D<double> azimuths;    ///< Azimuths in radians for all (el, az).
   std::vector<int> nr;                ///< Number of range bins for all (el).
   hoof::vector2D<double> ranges;      ///< Bin ranges in meters for all (el, r).
   std::vector<double> rscales;        ///< Range bin scales for all (el).  
   std::vector<double> rstarts;        ///< Range bin starts for all (el).  
   std::vector<double> vnys;           ///< Nyquist velocities for all (el).
   hoof::vector3D<double> meas;        ///< Measurements of DBZ or VRAD for all (el, az, r).
   hoof::vector3D<double> ths;         ///< Values of TH corresponding to DBZ for all (el, az, r).
   hoof::vector3D<double> quals;       ///< TOTAL quality values for all (el, az, r).
   hoof::vector3D<double> zs;          ///< Heights for all (el, az, r).  
};

#endif // HOOFMEASUREMENT_GUARD