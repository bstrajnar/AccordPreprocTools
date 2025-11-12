/**
   @file HoofWorker.cpp
   @author Peter Smerkol
   @brief Contains the HoofWorker class implementation.
*/

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <HoofSettings.h>
#include <HoofWorker.h>

using std::cout;
using std::endl;
using std::ofstream;
using std::string;
using std::vector;

/** 
   @brief Constructor.
*/
HoofWorker::HoofWorker()
{
   classMessage = "";
}

/**
   @brief Adds a warning.
   @param warn The warning string.
*/
void HoofWorker::warning(const string& warn)
{
   warnings.push_back(classMessage + " - " + warn);
}

/**
   @brief Adds an error.
   @param err The error string.
*/
void HoofWorker::error(const string& err)
{
   errors.push_back(classMessage + " - " + err);
}

/**
   @brief Outputs warnings and/or errors to console and/or log.
   @param logfile A stream from an opened log file to which to write to.
*/
void HoofWorker::output(ofstream& logfile)
{
   // output warnings
   for(int i=0; i<warnings.size(); i++)
   {
      if(HoofSettings::printLogWarnings)
         logfile << HoofSettings::warningTag << ": " << warnings[i] << endl;
      if(HoofSettings::printConsoleWarnings)
         cout << "    " << HoofSettings::warningTag << ": " << warnings[i] << endl;
   }

   // output errors
   for(int i=0; i<errors.size(); i++)
   {
      logfile << HoofSettings::errorTag << ": " << errors[i] << endl;
      if(HoofSettings::printConsoleErrors)
         cout << "    " << HoofSettings::errorTag << ": " << errors[i] << endl;
   }
}