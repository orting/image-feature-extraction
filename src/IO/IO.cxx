#include <fstream>
#include <stdexcept>

#include "ife/IO/IO.h"
#include "ife/Util/String.h"

template<>
float
parseElementFromString<char, float>(const std::string& s) {
  return std::stof(s);
}

template<>
double
parseElementFromString<char,double>(const std::string& s) {
  return std::stod(s);
}  


std::vector< StringPair >
readPairList( std::string inPath, char sep ) {
  std::ifstream is( inPath );
  std::vector< StringPair > pathPairs;
  std::string line;
  while ( is.good() ) {
    std::getline( is, line );

    // Skip blank lines
    if ( line.size() == 0 ) continue;

    auto pos = line.find_first_of( sep );
    if ( pos == std::string::npos ) {
      // Couldnt find the sep, so we cant split the line as a pair
      throw std::invalid_argument( "Line does not contain a separator" );
    }
    pathPairs.emplace_back( std::make_pair( trim( line.substr(0, pos) ),
					    trim( line.substr(pos + 1), " \r\n\t" ) ) );
  }
      
  return pathPairs;
}
