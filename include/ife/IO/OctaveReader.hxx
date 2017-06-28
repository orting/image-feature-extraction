#ifndef __OctaveReader_hxx
#define __OctaveReader_hxx

#include <stdexcept>
#include <algorithm>

#include <iostream>

#include "ife/IO/OctaveReader.h"
#include "ife/Util/String.h"

struct OctaveHeader {
  std::string creator;
  std::string name;
  std::string type;
  unsigned long dimensions;
  std::vector< unsigned long > size;
};

OctaveHeader ReadOctaveHeader(std::istream& is ) {
  OctaveHeader header;
  std::string line;
  std::getline( is, line );
  header.creator = trim( line, "# " );
  
  std::getline( is, line );
  auto kv = split( line, ':' );
  if ( kv.size() != 2 || trim(kv[0], "# ") != "name" ) {
    throw std::invalid_argument( "Expected '# name: <var-name>'" );
  }
  header.name = kv[1];

  std::getline( is, line );
  kv = split( line, ':' );
  if ( kv.size() != 2 || trim(kv[0], "# ") != "type" ) {
    throw std::invalid_argument( "Expected '# type: <type-name>'" );
  }
  header.type = kv[1];

  std::getline( is, line );
  kv = split( line, ':' );
  if ( kv.size() != 2 || trim(kv[0], "# ") != "ndims" ) {
    throw std::invalid_argument( "Expected '# ndims: <number-of-dimension>'" );
  }
  header.dimensions = std::stoul( kv[1] );

  std::getline( is, line );
  auto size = split( trim(line), ' ' );
  if ( size.size() != header.dimensions ) {
    std::cerr << size.size() << " : " << header.dimensions << std::endl;
    for ( auto s : size ) {
      std::cerr << "'" << s << "'" << std::endl;
    }
    throw std::invalid_argument( "ndims and number of size fields do not match" );
  }
  header.size.resize( header.dimensions );
 
  std::transform( size.begin(), size.end(),
		  header.size.begin(),
		  []( std::string s) { return std::stoul( s ); }
		  );

  if ( !is.good() ) {
    throw std::invalid_argument( "Error reading header" );
  }

  return header;
}

template<typename TPixel, unsigned long Dimension>
OctaveReader<TPixel, Dimension>
::OctaveReader(std::string path)
  : m_Path( path ),
    m_Read( false ),
    m_Image( nullptr )
{
  // Nothing to do
}

template<typename TPixel, unsigned long  Dimension>
typename OctaveReader<TPixel, Dimension>::ImagePointerType
OctaveReader<TPixel, Dimension>
::GetOutput() {
  if ( Dimension != 3 ) {
    // Fix this so it is handled on template level.
    throw std::invalid_argument( "Dimension must be 3" );
  }
  
  if ( ! m_Read ) {
    // Read the image
    // Get the header
    std::ifstream is( m_Path );
    OctaveHeader header = ReadOctaveHeader( is );

    // Check that we have the expected dimension
    if ( header.dimensions != Dimension ) {
      throw std::invalid_argument( "Dimension mismatch" );
    }

    // Setup the image info and allocate memory
    m_Image = ImageType::New();

    IndexType idx;
    idx.Fill(0);

    SizeType size;
    unsigned long numElements = 1;
    for ( unsigned long i = 0; i < Dimension; ++i ) {
      size[i] = header.size[i];
      numElements *= size[i];
    }
  
    m_Image->SetRegions( {idx, size } );
    m_Image->Allocate();
  
    // Read the data
    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType iter( m_Image, m_Image->GetRequestedRegion() );

    // Octave stores a 3D matrix in slice order, where each slice is stored in
    // column-major order.
    if ( std::any_of( header.size.cbegin(), header.size.cend(), [](const unsigned long&x) {
	  return x >= static_cast<unsigned long>( std::numeric_limits<long>::max() ); } ) ) {
      throw std::out_of_range( "Size of volume exceeds maximum index" );
    }
    long maxZ = static_cast< long >( header.size.at(2) );
    long maxY = static_cast< long >( header.size.at(1) );
    long maxX = static_cast< long >( header.size.at(0) );    
    for ( long z = 0; z < maxZ; ++z ) {	  
      for ( long x = 0; x < maxX; ++x ) {
	for ( long y = 0; y < maxY; ++y ) {
	  if ( !is.good() ) {
	    throw std::invalid_argument( "Not enough values in file" );
	  }
	  PixelType p;
	  is >> p;
	  IndexType idx{{x,y,z}};
	  m_Image->SetPixel( idx, p );
	}
      }
    }
    m_Read = true;
  }      
  return m_Image;
}

#endif
