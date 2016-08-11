#ifndef __HR2Reader_h
#define __HR2Reader_h

#include <string>
#include <vector>
#include <utility>

enum struct HR2Tag {
    PixelType,
    Compression,
    Dimension,
    Size,
    Origin,
    Spacing,
    ImageData
};

std::ostream& operator<<(std::ostream& os, const HR2Tag& tag);

enum struct HR2PixelType {
  Float
};

enum struct HR2Compression {
  ZLib
};

HR2PixelType stoHR2PT(std::string s);
HR2Compression stoHR2C(std::string s);

struct HR2Header {
  HR2PixelType pixelType;      // Pixel data type
  HR2Compression compression;  // Compression algorithm
  size_t dimension;            // Number of image dimensions
  size_t pixelDataLength;      // Bytes of pixel data
  std::vector<size_t> size;    // Size in each dimension
  std::vector<double> origin;  // Origin in each dimension
  std::vector<double> spacing; // Spacing in each dimension
};

std::pair< HR2Header,std::vector<float> > readHR2( std::string path );
void checkHeader( HR2Header header );
HR2Tag getTag(std::istream& is);
unsigned int getFieldLength(std::istream& is);
HR2Header readHR2Header( std::istream& is );
bool isHR2Format( std::istream& is );
#endif
