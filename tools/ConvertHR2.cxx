#include <iostream>
#include <vector>
#include <string>

#include "ife/IO/HR2Reader.h"

#include "itkImage.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIterator.h"

int main( int argc, char* argv[] ) {
  if ( argc < 3 ) {
    std::cerr << "Usage: <infile> <outfile>" << std::endl;
  }

  std::string infile(argv[1]);
  std::string outfile(argv[2]);

  HR2Header header;
  std::vector<float> buffer;
  
  try {
    auto res = readHR2(infile);
    header = res.first;
    buffer = res.second;
  }
  catch (std::exception &e) {
    std::cerr << "Error reading hr2 file" << std::endl
	      << "infile: " << infile << std::endl
	      << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if ( header.dimension != 3 ) {
    std::cerr << "Unexpected number of dimensions" << std::endl
	      << "Dimension: " << header.dimension << std::endl;
    return EXIT_FAILURE;
  }
  
  typedef itk::Image< float, 3 > ImageType;
  ImageType::Pointer image = ImageType::New();

  typedef typename ImageType::SizeType SizeType;
  typedef typename ImageType::IndexType IndexType;
  typedef typename ImageType::PointType PointType;
  typedef typename ImageType::SpacingType SpacingType;

  SpacingType spacing(&header.spacing[0]);
  PointType origin(&header.origin[0]);
  SizeType size{header.size[0],header.size[1],header.size[2]};
  IndexType index{0,0,0};

  std::cout << "Got header info:" << std::endl
	    << "Size: " << size << std::endl
	    << "Origin: " << origin << std::endl
	    << "Spacing: " << spacing << std::endl;
  
  image->SetOrigin( origin );
  image->SetSpacing( spacing );
  image->SetRegions( {index, size} );

  image->Allocate();

  typedef itk::ImageRegionIterator< ImageType > IteratorType;
  IteratorType iter( image, image->GetRequestedRegion() );

  auto bufferIter = buffer.begin();
  for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
    if ( bufferIter == buffer.end() ) {
      std::cerr << "Not enough values" << std::endl;
      return EXIT_FAILURE;
    }
    iter.Set( *bufferIter++ );
  }
  if ( bufferIter != buffer.end() ) {
    std::cerr << "Unused values" << std::endl;
    return EXIT_FAILURE;
  }

  typedef itk::ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outfile );
  writer->SetInput( image );

  try {
    writer->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to Update writer." << std::endl
	      << "infile: " << infile << std::endl
	      << "outfile: " << outfile << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
