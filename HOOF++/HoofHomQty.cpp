/**
   @file HoofHomQty.cpp
   @author Peter Smerkol
   @brief Contains the HoofHomQty class implementation.
*/

#include <string>
#include <vector>
#include <HoofAux.h>
#include <HoofHomQty.h>

using std::string;
using std::vector;

/**
  @brief Default constructor.
*/
HoofHomQty::HoofHomQty() {}

/**
   @brief Constructor.
   @param _name HOOF name of the quantity (DBZ, TH, VRAD, QUALITYn).
   @param _elAngle Elevation angle.
   @param _datetime Start datetime.
   @param _task Shortened HOOF task name.
   @param _oldDataset Dataset group name before homogenization.
   @param _oldData Data group name before homogenization.   
*/
HoofHomQty::HoofHomQty(const string& _name, double _elAngle, const string& _datetime, const string& _task,
   const string& _oldDataset, const string& _oldData)
{
   name = _name;
   elAngle = _elAngle;
   datetime = _datetime;
   task = _task;
   oldDataset = _oldDataset;
   oldData = _oldData;
}

/**
   @brief Definition of the less operator, for sorting the homogenization quantities by datetime.
   @param qty The HoofHomQty to compare with this one.
   @return True if this quantity's datetime is sooner that the compared one.
 */
bool HoofHomQty::operator < (const HoofHomQty& qty) const
{
   int year = HoofAux::to<int>(datetime.substr(0,4));
   int qYear = HoofAux::to<int>(qty.datetime.substr(0,4));
   int month = HoofAux::to<int>(datetime.substr(4,2));
   int qMonth = HoofAux::to<int>(qty.datetime.substr(4,2));
   int day = HoofAux::to<int>(datetime.substr(6,2));
   int qDay = HoofAux::to<int>(qty.datetime.substr(6,2));
   int hour = HoofAux::to<int>(datetime.substr(8,2));
   int qHour = HoofAux::to<int>(qty.datetime.substr(8,2));
   int minute = HoofAux::to<int>(datetime.substr(10,2));
   int qMinute = HoofAux::to<int>(qty.datetime.substr(10,2));
   int second = HoofAux::to<int>(datetime.substr(12,2));
   int qSecond = HoofAux::to<int>(qty.datetime.substr(12,2));

   if(year != qYear)
      return (year < qYear);
   else
   {
      if(month != qMonth)
         return (month < qMonth);
      else
      {
         if(day != qDay)
            return (day < qDay);
         else
         {
            if(hour != qHour)
               return (hour < qHour);
            else
            {
               if(minute != qMinute)
                  return (minute < qMinute);
               else
               {
                  if(second != qSecond)
                     return (second < qSecond);
                  else
                     return true;
               }
            }
         }
      }
   }

   return false;
}