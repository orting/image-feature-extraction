
#include <iostream>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkConstantPadImageFilter.h"

const std::string VERSION = "0.1";

int main(int argc, char* argv[]) {
  TCLAP::CmdLine cmd("Pad image to a given size with a constant value", ' ', VERSION);
  
  TCLAP::ValueArg<std::string> imageArg("i", "image", "Path to image", true, "", "path", cmd);
  TCLAP::ValueArg<std::string> outArg("o", "out", "Out path", true, "", "path", cmd);

  TCLAP::ValueArg<unsigned int> sizeXArg("x", "size-x", "size in x axis", true, 0, "unsigned int", cmd);
  TCLAP::ValueArg<unsigned int> sizeYArg("y", "size-y", "size in y axis", true, 0, "unsigned int", cmd);
//TCLAP::ValueArg<unsigned int> sizeZArg("z", "size-z", "size in z axis", false, 1, "unsigned int", cmd);

  TCLAP::ValueArg<short> padArg("p", "pad-value", "Value to use for padding", false, 0, "short", cmd);

  try {
    cmd.parse(argc, argv);
  }
  catch ( TCLAP::ArgException &e) {
    std::cerr << "Error : " << e.error()
	      << "for arg " << e.argId()
	      << std::endl;
    return EXIT_FAILURE;
  }

  const std::string imagePath( imageArg.getValue() );
  const std::string outPath( outArg.getValue() );
  const unsigned int sizeX( sizeXArg.getValue() );
  const unsigned int sizeY( sizeYArg.getValue() );
//const unsigned int sizeZ( sizeZArg.getValue() );
  const double padValue( padArg.getValue() );

  typedef short PixelType;
  const unsigned int Dimension = 2;
  typedef itk::Image<PixelType, Dimension> ImageType;

  typedef itk::ImageFileReader<ImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( imagePath );

  try {
    reader->Update();
  } 
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Error reading image." << std::endl
	      << "Image           : " << imagePath << std::endl
	      << "ExceptionObject : " << e << std::endl;
    return EXIT_FAILURE;
  }

  typedef itk::ConstantPadImageFilter<ImageType,ImageType> PadFilterType;
  PadFilterType::Pointer padFilter = PadFilterType::New();
  padFilter->SetInput( reader->GetOutput() );
  padFilter->SetConstant( padValue );

  ImageType::SizeType size = reader->GetOutput()->GetLargestPossibleRegion().GetSize();
  ImageType::SizeType padSize{{sizeX-size[0],sizeY-size[1]}}; //,sizeZ-size[2]}};
  ImageType::SizeType lowerBound, upperBound;
  for (size_t i = 0; i < Dimension; ++i ) {
    lowerBound[i] = padSize[i]/2;
    upperBound[i] = lowerBound[i] + padSize[i]%2;
  }
  
  padFilter->SetPadLowerBound( lowerBound );
  padFilter->SetPadUpperBound( upperBound );

  try {
    padFilter->Update();
  } 
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Error padding image." << std::endl
	      << "Image           : " << imagePath << std::endl
	      << "Size            : " << size << std::endl
	      << "PadSize         : " << padSize << std::endl
	      << "LowerBound      : " << lowerBound << std::endl
	      << "UpperBound      : " << upperBound << std::endl
	      << "ExceptionObject : " << e << std::endl;
    return EXIT_FAILURE;
  }


  typedef itk::ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput( padFilter->GetOutput() );
  writer->SetFileName( outPath );

  try {
    writer->Update();
  } 
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Error writing image." << std::endl
	      << "Image           : " << imagePath << std::endl
	      << "OutPath         : " << outPath << std::endl
	      << "ExceptionObject : " << e << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

