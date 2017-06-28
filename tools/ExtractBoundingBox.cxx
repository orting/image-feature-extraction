
// Find the bounding box of the image and extract it
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkClampImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageMaskSpatialObject.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  
  // Commandline parsing
  TCLAP::CmdLine cmd("Create a bag of instances samples from an image.", ' ', VERSION);

  //
  // Required arguments
  //
  
  TCLAP::ValueArg<std::string> 
    imageArg("i", "image", "Path to image.", true, "", "path", cmd);

  TCLAP::ValueArg<std::string> 
    maskArg("m", "mask", "Path to mask.", true, "", "path", cmd);
  
  TCLAP::ValueArg<std::string> 
    outArg("o", "out", "Path to output image", true, "", "path", cmd);

  
  try {
    cmd.parse(argc, argv);
  } catch(TCLAP::ArgException &e) {
    std::cerr << "Error : " << e.error() 
	      << " for arg " << e.argId() 
	      << std::endl;
    return EXIT_FAILURE;
  }

  // Store the arguments
  const std::string imagePath( imageArg.getValue() );
  const std::string maskPath( maskArg.getValue() );
  const std::string outPath( outArg.getValue() );

  //// Commandline parsing is done ////

  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef float PixelType;
  typedef unsigned char MaskPixelType;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::Image< MaskPixelType, Dimension >  MaskImageType;
  typedef typename MaskImageType::RegionType RegionType;
  

  // Read image
  typedef itk::ImageFileReader< ImageType > ImageReaderType;
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName( imagePath );

  typedef itk::ImageFileReader< MaskImageType > MaskReaderType;
  MaskReaderType::Pointer maskReader = MaskReaderType::New();
  maskReader->SetFileName( maskPath );

  try {
    imageReader->Update();
    maskReader->Update();
  }
  catch( itk::ExceptionObject &e ) {
    std::cerr << "Error reading images." << std::endl
	      << "Image path:      " << imagePath << std::endl
	      << "Mask path:       " << maskPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  

  // Calculate the bounding box
  // Convert to binary before finding bounding box
  typedef itk::ClampImageFilter< MaskImageType, MaskImageType > ClampFilterType;
  ClampFilterType::Pointer clampFilter = ClampFilterType::New();
  clampFilter->InPlaceOff();
  clampFilter->SetBounds(0, 1);
  clampFilter->SetInput( maskReader->GetOutput() );

  typedef itk::ImageMaskSpatialObject<Dimension> ImageMaskSpatialObject;
  ImageMaskSpatialObject::Pointer maskSO = ImageMaskSpatialObject::New();
  maskSO->SetImage ( clampFilter->GetOutput() );
  try {
    clampFilter->Update();
    maskSO->Update();
  }
  catch( itk::ExceptionObject &e ) {
    std::cerr << "Error calculating bounding box." << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  RegionType boundingBoxRegion  = maskSO->GetAxisAlignedBoundingBoxRegion();

  // Extract bounding box
  typedef itk::ExtractImageFilter< ImageType, ImageType > ExtractImageFilterType;
  ExtractImageFilterType::Pointer extractFilter = ExtractImageFilterType::New();
  extractFilter->SetInput( imageReader->GetOutput() );
  try {
    extractFilter->SetExtractionRegion( boundingBoxRegion );
  }
  catch (itk::ExceptionObject &e ) {
    std::cerr << "Error extracting region." << std::endl
	      << "Region:          " << boundingBoxRegion << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }


  // Write image
  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetInput( extractFilter->GetOutput() );
  writer->SetFileName( outPath );
  
  try {
    writer->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to write image." << std::endl
	      << "Out:             " << outPath << std::endl
	      << "Bounding box:    " << boundingBoxRegion << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
