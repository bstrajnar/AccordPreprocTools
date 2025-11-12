/**
   @file HoofAux.h
   @author Peter Smerkol
   @brief Contains definition of the HoofAux class.
*/

#ifndef HOOFAUX_GUARD
#define HOOFAUX_GUARD

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <type_traits>
#include <optional>
#include <limits>
#include <unordered_set>
#include <HoofTypes.h>

/**
   @class HoofAux
   @brief Class containing static helper functions used throughout HOOF.
*/
class HoofAux
{
   public:

      // members
      static const double dblEpsilon;                 ///< Epsilon for comparing double values.
      static const double earthRadius;                ///< Earth radius in meters.
      static const double eqEarthFactor;              ///< Factor in equivalent Earth calculation.
      static const double Pi;                         ///< Pi.

      /**
         @brief Converts a string to type T.
         @param s String to convert.
         @return The converted value.
      */
      template<typename T> static T to(const std::string& s)
      {
         std::string t = HoofAux::trim(s);
         T val;

         if constexpr (std::is_same_v<T,int>)
            val = std::stoi(t);
         else if constexpr (std::is_same_v<T,double>)
            val = std::stod(t);
         else if constexpr (std::is_same_v<T, bool>)
            val = (t == "T" || t == "TRUE");

         return val;
      }

      /**
         @brief Converts type T to a string.
         @param val Value to convert.
         @return The converted string.
      */
      template<typename T> static std::string string(const T& val)
      {
         std::string s;

         if(std::is_same<T, bool>::value)
         {
            if(val)
               s = "TRUE";
            else
               s = "FALSE";
         }
         else
            s = std::to_string(val);

         return s;
      }

      /**
         @brief Joins two string vector into one, keeping only unique values from each.
         @param vec1 First vector to join.
         @param vec2 Second vector to join.
         @return The joined vector.         
      */
      static std::vector<std::string> join(const std::vector<std::string>& vec1,
         const std::vector<std::string>& vec2)
      {
         std::unordered_set<std::string> unique;
         for(int i=0; i<vec1.size(); i++)
            unique.insert(vec1[i]);
         for(int i=0; i<vec2.size(); i++)
            unique.insert(vec2[i]);
         return std::vector<std::string>(unique.begin(), unique.end());
      } 

      /**
         @brief Optionally removes and/or replaces characters, then splits a string by whitespaces.
         @param s String to split.
         @param toRemove A string of characters to remove or replace.
            If 'None', no characters get removed or replaced.
         @param toReplace A string of characters to replace the characters in @param toRemove.
            If 'None', characters are not replaced.
      */
      static std::vector<std::string> split(const std::string& s, const std::string& toRemove = "None",
         const std::string& toReplace = "None")
      {
         std::string t = s;

         // first remove or replace 
         if(toRemove != "None")
         {
            if(toReplace == "None")
            {
               for(int i=0; i<toRemove.length(); i++)
                  t.erase(std::remove(t.begin(), t.end(), toRemove[i]), t.end());        
            }
            else
            {
               for(int i=0; i<toRemove.length(); i++)
                  std::replace(t.begin(), t.end(), toRemove[i], toReplace[i]);
            }
         }
   
         // convert to a vector of words
         std::vector<std::string> words;
         std::stringstream ss(t);
         std::string word;
         while(ss >> word)
            words.push_back(word);
         return words;  
      }

      /**
         @brief Trims a string of whitespaces and returns a new trimmed string.
         @param s String to trim.
         @return The trimmed string.
      */
      static std::string trim(const std::string& s)
      {
         std::string t = s;
         t.erase(0,t.erase(t.find_last_not_of(" ")+1).find_first_not_of(" "));
         return t;
      }
 
      /**
       * @brief Removes digits from a string.
       * @param s The string from which to remove digits.
       * @return The string with removed digits.
       */
      static std::string removeDigits(const std::string& s)
      {
         std::string t = s;
         t.erase(remove_if(t.begin(), t.end(), [](unsigned char c) { return isdigit(c);}), t.end());
         return t;
      }
      
      /**
         @brief Finds an element in a vector of type T.
         @param el Element to find.
         @param vec Vector in which to find the element.
         @return True if element is found, false if not.
      */
      template<typename T> static bool find(const T& el, const std::vector<T>& vec) 
      {
         if(std::find(vec.begin(), vec.end(), el) == vec.end())
            return false;
         else
            return true;
      }

      /**
         @brief Rounds a double to a precision.
         @param d Double to round.
         @param p The precision to round to.
         @return The rounded double.
      */
      static double round(double d, double p)
      {
         return std::round(d/p)*p;         
      }

      /**
         @brief Generates n evenly spaced values on interval [a,b) and fills a vector with them.
         @param vec The vector to fill.
         @param a Start of the interval.
         @param b Open end of the interval.
         @param n Number of values in vector to fill.
      */
      static void linspace(std::vector<double>& vec, double a, double b, int n)
      {
         if(n > vec.size() || n < 1)
            return;

         double step = (b-a)/(double)n;
         for(int i=0; i<n; i++)
            vec[i] = a + (double)i*step;
      }

      /**
         @brief Compare two doubles for equality.
         @param val1 First double to compare.
         @param val2 Second double to compare.
         @return True if they are equal up to a small epsilon, false if not.
      */
      static bool eqDbl(double val1, double val2)
      {
         return std::abs(val1 - val2) <= HoofAux::dblEpsilon;
      }

      /**
         @brief Compare two integers for equality.
         @param val1 First integer to compare.
         @param val2 Second integer to compare.
         @return True if they are equal, false if not.
      */
      static bool eqInt(double val1, double val2)
      {
         return val1 == val2;
      }

      /**
         @brief Replaces values equaling a value with a value in a 2D vector of type T
         @param vec Vector in which to replace values.
         @param cond Condition for replacing.
         @param value Value to replace.
      */
      template<typename T> static void replace(hoof::vector2D<T>& vec, const T& condValue, const T& value)
      {
         // select the equality fucntion
         bool (*eqFunc)(double, double);
         if constexpr (std::is_same_v<double, T>)
            eqFunc = &HoofAux::eqDbl;
         else
            eqFunc = reinterpret_cast<bool(*)(double,double)>(eqInt);
         
         // replace
         for(int i=0; i<vec.size(); i++)
         {
            for(int j=0; j<vec[i].size(); j++)
            {
               if(eqFunc(vec[i][j], condValue))
                  vec[i][j] = value;
            }
         }
      }

      /**
         @brief Replaces values equaling a value with a value in a 3D vector of type T
         @param vec Vector in which to replace values.
         @param cond Condition for replacing.
         @param value Value to replace.
      */
      template<typename T> static void replace(hoof::vector3D<T>& vec, const T& condValue, const T& value)
      {
         // select the equality fucntion
         bool (*eqFunc)(double, double);
         if constexpr (std::is_same_v<double, T>)
            eqFunc = &HoofAux::eqDbl;
         else
            eqFunc = reinterpret_cast<bool(*)(double,double)>(eqInt);

         // replace
         for(int i=0; i<vec.size(); i++)
         {
            for(int j=0; j<vec[i].size(); j++)
            {
               for(int k=0; k<vec[i][j].size(); k++)
               {
                  if(eqFunc(vec[i][j][k], condValue))
                     vec[i][j][k] = value;
               }
            }
         }
      }

      /**
         @brief Checks if all values in a 2D vector of doubles are NaN.
         @param vec The 2D vector to check.
         @return True if all values are NaN, false otherwise.
      */
      static bool isallnan(const hoof::vector2D<double>& vec) 
      {
         for(int i=0; i<vec.size(); i++)
         {
            for(int j=0; j<vec[i].size(); j++)
            {
               if(!std::isnan(vec[i][j]))
                  return false;
            }
         }
         return true;
      }

      /**
         @brief Checks if all values in a 3D vector of doubles are NaN.
         @param vec The 3D vector to check.
         @return True if all values are NaN, false otherwise.
      */
      static bool isallnan(const hoof::vector3D<double>& vec) 
      {
         for(int i=0; i<vec.size(); i++)
         {
            for(int j=0; j<vec[i].size(); j++)
            {
               for(int k=0; k<vec[i][j].size(); k++)
               {
                  if(!std::isnan(vec[i][j][k]))
                     return false;
               }
            }
         }
         return true;
      }

      /**
         @brief Finds minimum and maximum values in a 2D vector of doubles that can contain NaNs.
         @param vec The 2D vector to check.
         @return A (min, max) tuple, which is NaN if all are NaNs.
      */
      static hoof::Tuple nanminmax(const hoof::vector2D<double>& vec) 
      {
         double min = std::numeric_limits<double>::infinity();
         double max = -std::numeric_limits<double>::infinity();
         for(int i=0; i<vec.size(); i++)
         {
            for(int j=0; j<vec[i].size(); j++)
            {
               double value = vec[i][j];
               if(!std::isnan(value))
               {
                  if(value < min)
                     min = value;
                  if(value > max)
                     max = value;
               }
            }
         }
         if(std::isinf(min))
            min = hoof::dNaN;
         if(std::isinf(max))
            max = hoof::dNaN;
         hoof::Tuple result = {min, max};
         return result;
      }

      /**
         @brief Finds minimum and maximum values in a 3D vector of doubles that can contain NaNs.
         @param vec The 3D vector to check.
         @return A (min, max) tuple, which is NaN if all are NaNs.
      */
      static hoof::Tuple nanminmax(const hoof::vector3D<double>& vec) 
      {
         double min = std::numeric_limits<double>::infinity();
         double max = -std::numeric_limits<double>::infinity();
         for(int i=0; i<vec.size(); i++)
         {
            for(int j=0; j<vec[i].size(); j++)
            {
               for(int k=0; k<vec[i][j].size(); k++)
               {
                  double value = vec[i][j][k];
                  if(!std::isnan(value))
                  {
                     if(value < min)
                        min = value;
                     if(value > max)
                        max = value;
                  }
               }
            }
         }
         if(std::isinf(min))
            min = hoof::dNaN;
         if(std::isinf(max))
            max = hoof::dNaN;
         hoof::Tuple result = {min, max};
         return result;
      }

      /**
         @brief Fills an array with subset of values from a 3D double vector, where the subset
            indexes are given in a vector<Triple>.
         @param vec The 3D vector from which to get the values.
         @param idxs The vector<Triple> from which to get the indexes for the subset.
         @param arr The array to fill with the subset.
      */
      static void subset(const hoof::vector3D<double>& vec,
         const std::vector<hoof::Triple>& idxs, double* arr, std::size_t size)
      {
         for(int i=0; i<size; i++)
            arr[i] = vec[idxs[i][0]][idxs[i][1]][idxs[i][2]];
      }
};

#endif // HOOFAUX_GUARD