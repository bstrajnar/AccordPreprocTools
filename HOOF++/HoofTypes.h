/**
   @file HoofTypes.h
   @author Peter Smerkol
   @brief Contains a namespace with shorthand aliases for types used throughout HOOF.
*/

#ifndef HOOFTYPES_GUARD
#define HOOFTYPES_GUARD

#include <vector>
#include <array>
#include <chrono>
#include <limits>
#include <map>
#include <string>

namespace hoof
{
   // shorthands for 2D and 3D vectors of type T
   template<typename T> using vector2D = std::vector<std::vector<T>>;
   template<typename T> using vector3D = std::vector<std::vector<std::vector<T>>>;

   // shorthand for time related types
   using Clock = std::chrono::high_resolution_clock;
   using Time = std::chrono::time_point<Clock>;
   using Ms = std::chrono::milliseconds;

   // shorthands for NaN values
   constexpr double dNaN = std::numeric_limits<double>::quiet_NaN();
   constexpr int iNaN = std::numeric_limits<int>::quiet_NaN();

   // shorthand for a dictionary of vectors of objects of type T
   template<typename T> using VecDict = std::map<std::string, std::vector<T>>;
   template<typename T> using VecDictEl = std::pair<std::string,std::vector<T>>;

   // shorthand for a tuple and triple, represented by an array<double, 2> and array<int,3>
   using Tuple = std::array<double, 2>;
   using Triple = std::array<int, 3>;
}

#endif // HOOFTYPES_GUARD