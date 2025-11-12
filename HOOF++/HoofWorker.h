/**
   @file HoofWorker.h
   @author Peter Smerkol
   @brief Contains definition of HoofWorker class.
*/

#ifndef HOOFWORKER_GUARD
#define HOOFWORKER_GUARD

#include <string>
#include <vector>
#include <fstream>

/**
   @class HoofWorker
   @brief Class that handles output of warnings and errors.

   Serves as the base class for all worker objects.
*/
class HoofWorker
{
   public:
      // members
      std::string classMessage;          ///< String that gets added at the beginning of warnings and errors.
      std::vector<std::string> warnings; ///< Generated warning texts.
      std::vector<std::string> errors;   ///< Generated error texts.

      // constructor
      HoofWorker();
      // adds a warning
      void warning(const std::string& warn);        
      // adds an error
      void error(const std::string& err);
      // outputs warning and/or errors to console and/or log
      void output(std::ofstream& logfile);
};

#endif // HOOFWORKER_GUARD