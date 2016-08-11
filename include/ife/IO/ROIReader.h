#ifndef __ROIReader_h
#define __ROIReader_h

#include <vector>
#include <fstream>
#include <string>

template< typename TRegion >
class ROIReader {
public:
  typedef TRegion RegionType;
  typedef typename RegionType::SizeType SizeType;
  typedef typename RegionType::IndexType IndexType;

  static std::vector< RegionType > read( std::string path, bool header=true );
    
  template< typename OutputIter >
    static void read( std::string path, OutputIter it, bool header=true );
  
  template< typename OutputIter >
    static void read( std::istream& is, OutputIter it, bool header=true );
};

#include "ROIReader.hxx"
  
#endif
