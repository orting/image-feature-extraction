/* 
   Read Octave style ascii format

   It is assumed that the file conforms to the following format
   -----------------------------------------------------------------------------
   # Creator timestamp
   # name : <name of variable>
   # type : <type of variable>
   # ndims : <dimension of variable>
   <dim size 1> <dim size 2> ... <dim size n>
   <elem 1>
   <elem 2>
   ...
   <elem N>
   -----------------------------------------------------------------------------
*/
#ifndef __OctaveReader_h
#define __OctaveReader_h

#include <string>
#include "itkImage.h"

template<typename TPixel, unsigned long Dimension>
class OctaveReader {
public:
  typedef TPixel PixelType;
  typedef itk::Image<PixelType, Dimension> ImageType;
  typedef typename ImageType::Pointer ImagePointerType;

  typedef typename ImageType::IndexType IndexType;
  typedef typename ImageType::SizeType SizeType;
  
  OctaveReader(std::string path);

  ImagePointerType GetOutput();

private:
  std::string m_Path;
  bool m_Read;
  ImagePointerType m_Image;  
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "OctaveReader.hxx"
#endif

#endif
