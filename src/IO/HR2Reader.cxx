#include <utility>
#include <vector>
#include <string>
#include <iterator>
#include <stdexcept>
#include <fstream>
#include <iostream>

#include "ife/IO/HR2Reader.h"
#include "ife/Util/InflateStream.h"
/*
  Reverse engineering of HR2 format
  Numeric values in fields are stored as ascii separated by space \x20

  HR2
  <length-of-next-tag>
  PixelType<length of pixel type field>\x00
    <pixeltype>
  <length-of-next-tag>
  Dimension<length of dimension field>\x00
    <dimension>
  <length-of-next-tag>
  Size<length of size field>\x00
    <dim-1-size>\x20<dim-2-size>\x20 ...
  <length-of-next-tag>
  Origin<length of origin field>\x00
    <dim-1-origin>\x20<dim-2-origin>\x20 ...
  <length-of-next-tag>
  Spacing<length of spacing field>\x00
    <dim-1-spacing>\x20<dim-2-spacing>\x20 ...
  <length-of-next-tag>
  Compression<length-of-compression-field>\x00
    <compression-field>
  <length-of-next-tag>
  ImageData<length-of-image-data>\x00<image-data>

 */
std::ostream& operator<<(std::ostream& os, const HR2Tag& tag) {
  switch(tag) {
  case HR2Tag::PixelType:   os << "PixelType"; break;
  case HR2Tag::Compression: os << "Compression"; break;
  case HR2Tag::Dimension:   os << "Dimension"; break;
  case HR2Tag::Size:        os << "Size"; break;
  case HR2Tag::Origin:      os << "Origin"; break;
  case HR2Tag::Spacing:     os << "Spacing"; break;
  case HR2Tag::ImageData:   os << "ImageData"; break;
  default:                  os << "UnknownTag";
  }
  return os;
}

std::pair< HR2Header, std::vector<float> >
readHR2( std::string path ) {
  std::ifstream is( path );
  if ( ! is.good() ) {
    throw std::runtime_error("Could not read file");
  }

  if ( !isHR2Format( is ) ) {
    throw std::invalid_argument("Not an HR2 file");
  }
  
  // Read and check the header
  HR2Header header = readHR2Header( is );
  checkHeader( header );

  // Now we can try to read the data
  std::vector< unsigned char > inflated;
  if ( inflateStream( is, std::back_inserter(inflated) ) != Z_OK ) {
    throw std::runtime_error( "Error inflating" );
  }

  // Convert to float
  std::vector< float > buffer( inflated.size() / sizeof(float) );
  std::copy( inflated.begin(),
	     inflated.end(),
	     reinterpret_cast<unsigned char*>(&buffer[0]) );
  
  return std::make_pair(header, buffer);
}

bool isHR2Format( std::istream& is ) {
  // Must start with the string "HR2"
  char buf[3];
  is.read(buf, 3);
  return buf[0] == 'H' && buf[1] == 'R' && buf[2] != '3';
}

HR2Header readHR2Header( std::istream& is ) {  
  HR2Header header;
  while ( is.good() ) {
    HR2Tag tag = getTag(is);
    unsigned int len = getFieldLength(is);

    if ( tag == HR2Tag::ImageData ) {
      // If we have found the ImageData we are done reading the header
      header.pixelDataLength = len;
      break;
    }

    size_t pos;
    std::string s(len, 0);
    is.read(&s[0], len );
    switch ( tag ) {
    case HR2Tag::PixelType:
      // Read a string naming the datatype
      header.pixelType = stoHR2PT(s);
      break;
	
    case HR2Tag::Dimension:
      // Read a single unsigned integer stored as ascii
      header.dimension = std::stoul(s);
      break;

    case HR2Tag::Size:
      // Read a series of unsigned integers separated by <space>
      while (s.size() > 0) {
	header.size.push_back( std::stoul(s, &pos) );
	s = s.substr(pos);
      }
      break;
	
    case HR2Tag::Origin:
      // Read a series of unsigned integers separated by <space>
      while (s.size() > 0) {
	header.origin.push_back( std::stod(s, &pos) );
	s = s.substr(pos);
      }
      break;

    case HR2Tag::Spacing:
      // Read a series of unsigned integers separated by <space>
      while (s.size() > 0) {
	header.spacing.push_back( std::stod(s, &pos) );
	s = s.substr(pos);
      }
      break;

    case HR2Tag::Compression:
      // Read a string naming the compression method
      header.compression = stoHR2C(s);
      break;

    default:
      throw std::invalid_argument("Not an HR2 tag");
    }
  }

  if ( ! is.good() ) {
    throw std::runtime_error("Error reading from stream");
  }
  
  return header;
}

// Check that header tags are consistent and supported
void checkHeader( HR2Header header ) {
  if ( header.pixelType != HR2PixelType::Float ) {
    throw std::runtime_error("Only PixelType float implemented");
  }

  if ( header.compression != HR2Compression::ZLib ) {
    throw std::runtime_error("Only ZLib compression implemented");
  }

  if ( header.size.size() != header.dimension ) {
    throw std::runtime_error("Number of size elements does not match dimension");
  }

  if ( header.origin.size() != header.dimension ) {
    throw std::runtime_error("Number of origin elements does not match dimension");
  }

  if ( header.spacing.size() != header.dimension ) {
    throw std::runtime_error("Number of spacing elements does not match dimension");
  }
}


HR2Tag getTag(std::istream& is) {
  unsigned int len = is.get();
  std::string s(len, 0);
  is.read(&s[0], len);
  if (s == "PixelType")   return HR2Tag::PixelType;
  if (s == "Compression") return HR2Tag::Compression;
  if (s == "Dimension")   return HR2Tag::Dimension;
  if (s == "Size")        return HR2Tag::Size;
  if (s == "Origin")      return HR2Tag::Origin;
  if (s == "Spacing")     return HR2Tag::Spacing;
  if (s == "ImageData")   return HR2Tag::ImageData;

  throw std::invalid_argument("Not an HR2 tag");
}

unsigned int getFieldLength(std::istream& is) {
  char byte;
  std::vector<unsigned int> bytes;
  while( (byte = is.get()) ) {
    bytes.push_back(byte);
    if ( bytes.size() == 4 ) { break; }
  }
  while ( bytes.size() < 4 ) {
    bytes.push_back(0);
  }
  return (bytes[3]<<24) | (bytes[2]<<16) | (bytes[1]<<8) | bytes[0];
}

HR2PixelType stoHR2PT(std::string s) {
  if (s == "float") {
    return HR2PixelType::Float;
  }
  throw std::invalid_argument("Unknown PixelType: '" + s + "'");
}   

HR2Compression stoHR2C(std::string s) {
  if (s == "ZLib") {
    return HR2Compression::ZLib;
  }
  throw std::invalid_argument("Unknown Compression: '" + s + "'");
}

