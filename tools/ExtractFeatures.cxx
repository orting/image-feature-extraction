
#include <iostream>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkClampImageFilter.h"

#include "ife/Filters/ImageToEmphysemaFeaturesFilter.h"
#include "ife/Util/Path.h"

const std::string VERSION("0.1");
const std::string OUT_FILE_TYPE(".nii.gz");

int main( int argc, char* argv[] ) {
    // Commandline parsing
  TCLAP::CmdLine cmd("Create a bag of instances samples from an image.", ' ', VERSION);

  //
  // Required arguments
  //
  
  // We need a single image
  TCLAP::ValueArg<std::string> 
    imageArg("i", 
	     "image", 
	     "Path to image.",
	     true,
	     "",
	     "path", 
	     cmd);

  // We need a single mask
  TCLAP::ValueArg<std::string> 
    maskArg("m", 
	    "mask", 
	    "Path to mask.",
	    true,
	    "",
	    "path", 
	    cmd);

  
  // We need a directory for storing the ROIs
  TCLAP::ValueArg<std::string> 
    outArg("o", 
	   "out", 
	   "Base output path",
	   true, 
	   "", 
	   "path", 
	   cmd);

  // We need scales for the multi scale features
  TCLAP::MultiArg<float> 
    scalesArg("s", 
	      "scale", 
	      "Scales for the Gauss applicability function",
	      true, 
	      "double", 
	      cmd);
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
  const std::string outBasePath( outArg.getValue() );
  const std::vector< float > scales( scalesArg.getValue() );
  //// Commandline parsing is done ////
  
  
  typedef float PixelType;
  typedef unsigned char MaskPixelType;
  const unsigned int Dimension = 3;
  typedef itk::Image< PixelType, Dimension > ImageType;
  typedef itk::Image< MaskPixelType, Dimension > MaskType;
  typedef itk::VectorImage< PixelType, Dimension >  VectorImageType;
  

  // Setup the reader
  typedef itk::ImageFileReader< ImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( imagePath );

  typedef itk::ImageFileReader< MaskType > MaskReaderType;
  MaskReaderType::Pointer maskReader = MaskReaderType::New();
  maskReader->SetFileName( maskPath );

  // Setup the clamp filter
  typedef itk::ClampImageFilter< MaskType,
				 MaskType > ClampFilterType;
  ClampFilterType::Pointer clampFilter = ClampFilterType::New();
  clampFilter->InPlaceOn();
  clampFilter->SetBounds(0, 1);
  clampFilter->SetInput( maskReader->GetOutput() );

  
  // Setup the feature filter
  typedef itk::ImageToEmphysemaFeaturesFilter<
    ImageType,
    MaskType,
    VectorImageType > FeatureFilterType;
  FeatureFilterType::Pointer featureFilter = FeatureFilterType::New();
  featureFilter->SetInputImage( reader->GetOutput() );
  featureFilter->SetInputMask( clampFilter->GetOutput() );
  
  typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ImageType>
    IndexSelectionType;
  IndexSelectionType::Pointer indexSelectionFilter = IndexSelectionType::New();
  indexSelectionFilter->SetInput( featureFilter->GetOutput() );
  
  // Setup the writer
  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetInput( indexSelectionFilter->GetOutput() );

  std::vector< std::string > featureNames{
    "GaussianBlur", "GradientMagnitude",
      "Eigenvalue1", "Eigenvalue2", "Eigenvalue3",
      "LaplacianOfGaussian", "GaussianCurvature", "FrobeniusNorm"
      };
  
  for ( auto scale : scales ) {
    featureFilter->SetSigma( scale );

    for (unsigned int i = 0; i < featureNames.size(); ++i ) {
      indexSelectionFilter->SetIndex( i );
      std::string outPath = outBasePath
	+ "_scale_" + std::to_string(scale)
	+ featureNames[i] + OUT_FILE_TYPE;
      writer->SetFileName( outPath );
      try {
	featureFilter->UpdateLargestPossibleRegion();
	writer->Update();
      }
      catch ( itk::ExceptionObject &e ) {
	std::cerr << "Failed to process." << std::endl
		  << "Image: " << imagePath << std::endl
		  << "Mask: " << maskPath << std::endl
		  << "Out: " << outPath << std::endl
		  << "ExceptionObject: " << e << std::endl;
	return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
