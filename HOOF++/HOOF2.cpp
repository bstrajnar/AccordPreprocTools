#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <stdexcept>
#include <execinfo.h>
#include <HoofTypes.h>
#include <HoofAux.h>
#include <HoofSettings.h>
#include <HoofWorker.h>
#include <HoofData.h>
#include <HoofH5File.h>
#include <HoofHomogenizer.h>
#include <HoofDealiaser.h>
#include <HoofSuperober.h>

using std::string;
using std::cout;
using std::endl;
using std::filesystem::directory_iterator;
using std::filesystem::file_size;
using std::filesystem::remove;
using std::ofstream;
using std::chrono::duration_cast;
using namespace hoof;

/**
   \mainpage HOOF2 C++ documentation

   \author Peter Smerkol

   An implementation of HOOF2.py 1.10 in C++

   \section req Requirements:
   - gcc compiler that supports C++17 (from GCC 7.1 on),
   - gsl library 2.7.1,
   - HDF5 library 1.10.10

   \section comp Compiling:
   h5c++ -o HOOF2 -I. Hoof*.cpp -lgsl HOOF2.cpp -O2

   \section run Running:
   ./HOOF2 <namelistfile> <input folder> <output folder>

   \section other Other:
   Last five characters of the file name has to contain the radar site name as defined by OPERA
*/

/**
   @brief Prints the stack trace.
*/
void printStack()
{
   const int maxLines = 100;
   void* addrlist[maxLines];
   int addrlen = backtrace(addrlist, maxLines);
   if (addrlen == 0) {
      std::cerr << "  <empty stack>" << endl;
      return;
   }
   char** symbollist = backtrace_symbols(addrlist, addrlen);
   std::cerr << "Stack trace:" << endl;
   for (int i = 0; i < addrlen; ++i) {
      std::cerr << symbollist[i] << endl;
   }
   free(symbollist);
}

/**
   @brief Helper function that handles errors. It writes error to output and closes all open files.
   @param worker The worker object to handle. 
   @param inFile The input file to close.
   @param  outFile The output file to close.
   @param logFile The log file to write output to and close.
   @return True if errors occured, otherwise false.
 */
bool handleErrors(HoofWorker& worker, HoofH5File& inFile, HoofH5File& outFile, ofstream& logFile)
{
   if(worker.errors.size() != 0)
   {
      worker.output(logFile);
      outFile.close();
      inFile.close();
      logFile.close();
      return true;
   }
   return false;
}

// ---------------------------------------------------------------------------------
// -------------------- main function ----------------------------------------------
// ---------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
   // if HOOF is called wrongly, output instructions and exit
   if(argc != 4)
   {
      cout << "Wrong number of command line arguments, the syntax is:" << endl;
      cout << "./HOOF2 <namelist file> <input folder> <output folder>" << endl;
      cout << "Last five characters of the file name has to contain the radar site name as defined by OPERA." << endl;
      return -1;   
   }

   // get command line arguments and parse the settings
   string namelist = argv[1];
   string inFolder = argv[2];
   string outFolder = argv[3];
   HoofSettings settings(namelist, inFolder, outFolder);

   // get start time
   Clock clock;
   Time startTime = clock.now();

   // get files in the input folder and loop on them
   int allFiles = 0;
   int goodFiles = 0;
   for(auto& entry : directory_iterator(inFolder))
   {
      // go only through files that have the correct extensions
      bool extFound = false;
      for(int i=0; i<HoofSettings::fileExtensions.size(); i++)
      {
         if(entry.path().extension() == HoofSettings::fileExtensions[i])
         {
            extFound = true;
            break;
         }
      }
      if(!extFound)
         continue;
      allFiles++;
      
      // --- determine file paths and open the log file
      string stem = entry.path().stem().string();
      string fileName = entry.path().filename().string();
      string inFilePath = HoofSettings::inFolder + fileName;
      string outFilePath = HoofSettings::outFolder + fileName;
      string logFilePath = HoofSettings::outFolder + stem + ".log";
      ofstream logFile(logFilePath);
      cout << "--------------- processing file " << fileName << endl; 

      // --- initialize timers and get beginning time
      Time beginTime = clock.now();
      Time timer[15];

      // --- open the data object, determine the site name and open the input and output HDF5 files
      timer[0] = clock.now();
      cout << "Reading input file ..." << endl;
      HoofData data;
      data.site = stem.substr(stem.length()-5);
      HoofH5File inFile(inFilePath.c_str(), "read");
      HoofH5File outFile(outFilePath.c_str(), "write");
      timer[1] = clock.now();  

      try
      {
      // --- homogenize data
      cout << "Homogenizing data ..." << endl;
      HoofHomogenizer homogenizer(inFile, outFile, data);
      homogenizer.sort();
      timer[2] = clock.now();
  
      // check that required attributes are present in homogenized data
      cout << "Checking and writing homogenized data to file ..." << endl;
      homogenizer.checkAndWrite();
      if(handleErrors(homogenizer, inFile, outFile, logFile))
         continue;
      timer[3] = clock.now();
  
      // write the homogenized data needed by dealiasing and superobing to the data object
      if(HoofSettings::dealiasing || HoofSettings::superobing)
      {
         cout << "Storing homogenized data for further use ..." << endl;
         homogenizer.storeData();
         if(handleErrors(homogenizer, inFile, outFile, logFile))
            continue;
         timer[4] = clock.now(); 
      }
 
      // write warnings from homogenization to log
      cout << "Writing warnings to log ..." << endl;
      homogenizer.output(logFile);

      // dealiasing
      if(HoofSettings::dealiasing)
      {
         // check if VRAD data is ok for dealiasing
         cout << "Checking VRAD data for dealiasing ..." << endl;
         HoofDealiaser dealiaser(data, outFile);
         dealiaser.checkData();
         timer[5] = clock.now();

         // calculate quantities used in the minimization to get the wind model
         cout << "Calculating wind model quantities ..." << endl;
         dealiaser.calculateWindModelQtys();
         timer[6] = clock.now();

         // determine height sectors
         cout << "Determining height sectors ..." << endl;
         dealiaser.determineHeightSectors();
         timer[7] = clock.now();

         // calculate wind models
         cout << "Calculating wind models ..." << endl;
         dealiaser.calculateWindModels();
         timer[8] = clock.now();

         // dealias
         cout << "Dealiasing ..." << endl;
         dealiaser.dealias();
         timer[9] = clock.now();   
         
         // write dealiased data
         cout << "Writing dealiased data to file ..." << endl;
         dealiaser.write();
         timer[10] = clock.now();

         // write warnings from dealiasing to log
         cout << "Writing warnings to log ..." << endl;
         dealiaser.output(logFile);
      }

      // superobing
      if(HoofSettings::superobing)
      {
         // check if data is ok for superobing
         cout << "Checking data for superobing ..." << endl;
         HoofSuperober superober(data, outFile);
         superober.checkData();
         timer[11] = clock.now();

         // prepare superobed metadata
         cout << "Preparing superobed metadata ..." << endl;
         superober.prepareMetadata();
         timer[12] = clock.now();

         // superob
         cout << "Superobing ..." << endl;
         superober.superob();
         timer[13] = clock.now();         

         // write superobed data
         cout << "Writing superobed data ..." << endl;
         superober.write();
         timer[14] = clock.now();  
      }
      }
      catch(const std::exception& e)
      {
         cout << "Unknown error: " << e.what() << endl;
         printStack();
         continue;
      }
      catch(...)
      {
         cout << "Unknown error " << endl;
         printStack();
         continue;
      }

      // print timings
      Time finishTime = clock.now();
      if(HoofSettings::printConsoleTiming)
      {
         cout << "Timings:" << endl;
         cout << "   Input file reading:             " <<
            duration_cast<Ms>(timer[1]-timer[0]).count() << " ms" << endl;
         cout << "   Homogenization:                 " <<
            duration_cast<Ms>(timer[2]-timer[1]).count() << " ms" << endl;
         cout << "   Homogenization check/write:     " <<
            duration_cast<Ms>(timer[3]-timer[2]).count() << " ms" << endl;
         if(HoofSettings::dealiasing || HoofSettings::superobing)
            cout << "   Storing homogenized data:       " <<
               duration_cast<Ms>(timer[4]-timer[3]).count() << " ms" << endl;
         if(HoofSettings::dealiasing)
         {            
            cout << "   Checking dealiasing data:       " <<
               duration_cast<Ms>(timer[5]-timer[4]).count() << " ms" << endl;
            cout << "   Calculating wind model theory:  " <<
               duration_cast<Ms>(timer[6]-timer[5]).count() << " ms" << endl;  
            cout << "   Determining height sectors:     " <<
               duration_cast<Ms>(timer[7]-timer[6]).count() << " ms" << endl;
            cout << "   Calculating wind models:        " <<
               duration_cast<Ms>(timer[8]-timer[7]).count() << " ms" << endl;                             
            cout << "   Dealiasing:                     " <<
               duration_cast<Ms>(timer[9]-timer[8]).count() << " ms" << endl;   
            cout << "   Writing dealiased data:         " <<
               duration_cast<Ms>(timer[10]-timer[9]).count() << " ms" << endl;
         }
         if(HoofSettings::superobing)
         {
            if(HoofSettings::dealiasing)
               cout << "   Checking superobing data:       " <<
                  duration_cast<Ms>(timer[11]-timer[10]).count() << " ms" << endl;
            else
               cout << "   Checking superobing data:       " <<
                  duration_cast<Ms>(timer[11]-timer[4]).count() << " ms" << endl;
            cout << "   Preparing superobed metadata:   " <<
               duration_cast<Ms>(timer[12]-timer[11]).count() << " ms" << endl;  
            cout << "   Superobing:                     " <<
               duration_cast<Ms>(timer[13]-timer[12]).count() << " ms" << endl;
            cout << "   Writing superobed data:         " <<
               duration_cast<Ms>(timer[14]-timer[13]).count() << " ms" << endl;                                  
         }
      }

      // close the files and remove the log file if empty
      goodFiles++;
      logFile.close();
      inFile.close();
      outFile.close();
      if(file_size(logFilePath) == 0)
         remove(logFilePath);
      Time endTime = clock.now();
      cout << "Analysis time:   " << duration_cast<Ms>(endTime - beginTime).count() << " ms" << endl;
   }

   Time endTime = clock.now();
   cout << "HOOF succesfully analysed " << goodFiles << " out of " << allFiles << " files in " << 
      duration_cast<Ms>(endTime-startTime).count() << " ms" << endl;

   return 0;
}
