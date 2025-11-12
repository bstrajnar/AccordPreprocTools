/**
   @file HoofSettings.cpp
   @author Peter Smerkol
   @brief Contains the HoofSettings class implementation.
*/

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <HoofTypes.h>
#include <HoofAux.h>
#include <HoofNamAtt.h>
#include <HoofSettings.h>

using std::string;
using std::vector;
using std::ifstream;
using std::find;
using namespace hoof;

/**
   @brief Constructor, reads command line parameters and namelist and fills the class.
   @param _namelist Name of the namelist file from command line.
   @param _inFolder Relative path to the folder containing radar files from command line.
   @param _outFolder Relative path to the folder which contains output from commandline.
*/
HoofSettings::HoofSettings(const string& _namelist, const string& _inFolder, const string& _outFolder)
{
   // save the command line arguments
   inFolder = _inFolder;
   outFolder = _outFolder;
   namelist = _namelist;

   // open the namelist file, find keyword indexes and store lines
   ifstream namelistFile(namelist.c_str());
   vector<string> lines;
   string line;
   vector<int> keywordIndexes;
   int idx = 0;
   while(getline(namelistFile, line))
   {
      // leave out commented lines
      if(line.at(line.find_first_not_of(' ')) == '#')
         continue;
      // store the index of keyword
      if(line.at(0) == '[')
         keywordIndexes.push_back(idx);
      // store the current line
      lines.push_back(line);
      idx++;
   }
   keywordIndexes.push_back(lines.size());

   // go through namelist lines and fill members according to keywords
   for(int i=0; i<keywordIndexes.size()-2; i++)
   {
      // get current and next keyword index
      int cidx = keywordIndexes[i];
      int nidx = keywordIndexes[i+1];
        
      // fill data according to keywords 
      if(lines[cidx] == "[File extensions to read]")
         fileExtensions = HoofAux::split(lines[cidx+1], "{}");
      if(lines[cidx] == "[Log keywords]")
      {
         for(int j=cidx+1; j<nidx; j++)
         {
            vector<string> words = HoofAux::split(lines[j]);
            if(words[0] == "WarningTag")
               warningTag = words[2];
            if(words[0] == "ErrorTag")
               errorTag = words[2];   
         }
      }
      if(lines[cidx] == "[Print warnings to console]")
         printConsoleWarnings = HoofAux::to<bool>(lines[cidx+1]);
      if(lines[cidx] == "[Print errors to console]")
         printConsoleErrors = HoofAux::to<bool>(lines[cidx+1]);
      if(lines[cidx] == "[Print warnings to log]")
         printLogWarnings = HoofAux::to<bool>(lines[cidx+1]);
      if(lines[cidx] == "[Print timing to console]")
         printConsoleTiming = HoofAux::to<bool>(lines[cidx+1]);
      if(lines[cidx] == "[Radar moment names to save]")
      {
         for(int j=cidx+1; j<nidx; j++)
         {
            vector<string> words = HoofAux::split(lines[j], "{}");
            if(words[0] == "DBZ")
               for(int k=2; k<words.size(); k++)
                  dbzNames.push_back(words[k]);
            if(words[0] == "TH")
               for(int k=2; k<words.size(); k++)
                  thNames.push_back(words[k]);
            if(words[0] == "VRAD")
               for(int k=2; k<words.size(); k++)
                  vradNames.push_back(words[k]);
         }
      }
      if(lines[cidx] == "[Required DBZ moment quality groups]")
         dbzQualNames = HoofAux::split(lines[cidx+1], "{}");
      if(lines[cidx] == "[Common attributes and default values]")
      {
         for(int j=cidx+1; j<nidx; j++)
            comAtts.push_back(HoofNamAtt(lines[j]));
      } 
      if(lines[cidx].find("[Specific attributes and default values -") != std::string::npos)
      {            
         string site = HoofAux::split(lines[cidx], "[]").back();
         vector<HoofNamAtt> atts;
         for(int j=cidx+1; j<nidx; j++)
            atts.push_back(HoofNamAtt(lines[j]));  
         specAtts.insert(VecDictEl<HoofNamAtt>(site, atts));       
      }
      if(lines[cidx] == "[Dealiasing]")
         dealiasing = HoofAux::to<bool>(lines[cidx+1]);
      if(lines[cidx] == "[Height sector size in m]")
         zSectorSize = HoofAux::to<double>(lines[cidx+1]); 
      if(lines[cidx] == "[Maximum height]")
         zMax = HoofAux::to<double>(lines[cidx+1]);       
      if(lines[cidx] == "[Minimum good points in height sector]")
         minGoodPoints = HoofAux::to<int>(lines[cidx+1]);    
      if(lines[cidx] == "[Maximum dealiased wind speed in m/s]")
         maxWind = HoofAux::to<double>(lines[cidx+1]);   
      if(lines[cidx] == "[Superobing]")
         superobing = HoofAux::to<bool>(lines[cidx+1]);
      if(lines[cidx] == "[Range bin factor]")
         rangeBinFactor = HoofAux::to<int>(lines[cidx+1]);
      if(lines[cidx] == "[Ray angle factor]")
         rayAngleFactor = HoofAux::to<int>(lines[cidx+1]);
      if(lines[cidx] == "[Max arc size in m]")
         maxArcSize = HoofAux::to<double>(lines[cidx+1]);
      if(lines[cidx] == "[DBZ min quality]")
         minQuality = HoofAux::to<double>(lines[cidx+1]);
      if(lines[cidx] == "[DBZ clear sky threshold]")
         dbzClearsky = HoofAux::to<double>(lines[cidx+1]);
      if(lines[cidx] == "[DBZ min percentage of good points]")
         dbzPercentage = HoofAux::to<double>(lines[cidx+1]);
      if(lines[cidx] == "[VRAD min percentage of good points]")
         vradPercentage = HoofAux::to<double>(lines[cidx+1]);
      if(lines[cidx] == "[VRAD max standard deviation]")
         vradMaxStd = HoofAux::to<double>(lines[cidx+1]);
   }
}

// --- initialize all static members
string HoofSettings::inFolder = " ";
string HoofSettings::outFolder = "";
string HoofSettings::namelist = "";
vector<string> HoofSettings::fileExtensions;
string HoofSettings::warningTag = "";
string HoofSettings::errorTag = "";
bool HoofSettings::printConsoleWarnings = false;
bool HoofSettings::printLogWarnings = false;
bool HoofSettings::printConsoleErrors = false;
bool HoofSettings::printConsoleTiming = false;
vector<string> HoofSettings::dbzNames;
vector<string> HoofSettings::thNames;
vector<string> HoofSettings::vradNames;
vector<string> HoofSettings::dbzQualNames;
vector<HoofNamAtt> HoofSettings::comAtts;
VecDict<HoofNamAtt> HoofSettings::specAtts;
bool HoofSettings::dealiasing = false;
double HoofSettings::zSectorSize = 0.0;
double HoofSettings::zMax = 0.0;
int HoofSettings::minGoodPoints = 0;
double HoofSettings::maxWind = 0.0;
bool HoofSettings::superobing = false;
int HoofSettings::rangeBinFactor = 0;
int HoofSettings::rayAngleFactor = 0;
double HoofSettings::maxArcSize = 0.0;
double HoofSettings::minQuality = 0.0;
double HoofSettings::dbzClearsky = 0.0;
double HoofSettings::dbzPercentage = 0.0;
double HoofSettings::vradPercentage = 0.0;
double HoofSettings::vradMaxStd = 0.0;