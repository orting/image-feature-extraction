/* 
   Perform normalized convolution of a 0'th order filter with an image and 
   optionally mask the result with the certainty, such that all voxels with 0 
   certainty are set to 0.
   See:
   Knutsson, Hans and Westin, Carl-Fredrik
   Normalized and Differential Convolution.
   (Specifically section 3.2 0:th order interpolation)
   
   Let B be the filter, T the image, c the certainty of the image and a the
   applicability of the filter.
   
   Let B=1 and denote convolution by *, then we have
   U_N = {a * cT}_N = {a * c}^-1 {a * cT} = {a * cT}/{a * c}.
   
   If we set a = Gauss_sigma and c = binary mask representing a region of 
   interest (ROI), then we get convolution of a Gaussian at scale sigma with T,
   where the influence of values outside the ROI has been reduced   

   Note To Self:
   Should normalization of smoothing be with 1/s² or 1/s? See ITK Software Guide
   version 4.7 page 101 for a discussion.

 */
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkMaskImageFilter.h"

#include "ife/Filters/NormalizedGaussianConvolutionImageFilter.h"
#include "ife/Util/Path.h"

const std::string VERSION("0.1");
const std::string OUT_FILE_TYPE(".nii.gz");

int main(int argc, char *argv[]) {
  // Commandline parsing
  TCLAP::CmdLine cmd("Perform normalized convolution.", ' ', VERSION);

  // We need a single image
  TCLAP::ValueArg<std::string> 
    imageArg("i", 
	     "image", 
	     "Path to image (T)",
	     true,
	     "",
	     "path", 
	     cmd);

  // We need a single certainty mask
  TCLAP::ValueArg<std::string> 
    certaintyArg("c", 
		 "certainty", 
		 "Path to certainty mask (c). Must match image dimensions.",
		 true,
		 "",
		 "path", 
		 cmd);
  

  // We need scales for the Gauss applicability function
  TCLAP::MultiArg<double> 
    scalesArg("s", 
	      "scale", 
	      "Scales for the Gauss applicability function",
	      true, 
	      "double", 
	      cmd);

  // We need a directory for storing the resulting images
  TCLAP::ValueArg<std::string> 
    outDirArg("o", 
	       "outdir", 
	       "Path to output directory",
	       true, 
	       "", 
	       "path", 
	       cmd);

  // We can use a prefix to generate filenames
  TCLAP::ValueArg<std::string> 
    prefixArg("p", 
	      "prefix", 
	      "Prefix to use for output filenames",
	      false, 
	      "normconv_", 
	      "string", 
	      cmd);

  // Should the output be masked after convolution?
  TCLAP::ValueArg<bool> 
    maskOutputArg("m", 
		  "maskoutput", 
		  "Mask the output after convolution.",
		  false, 
		  false, 
		  "boolean", 
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
  std::string imagePath( imageArg.getValue() );
  std::string certaintyPath( certaintyArg.getValue() );
  std::vector<double> scales( scalesArg.getValue() );
  std::string outDirPath( outDirArg.getValue() );
  std::string prefix( prefixArg.getValue() );
  const bool maskOutput( maskOutputArg.getValue() );

  //// Commandline parsing is done ////


  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef float PixelType;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::ImageFileReader< ImageType > ReaderType;

    
  // Setup the readers
  ReaderType::Pointer imageReader = ReaderType::New();
  imageReader->SetFileName( imagePath );

  ReaderType::Pointer certaintyReader = ReaderType::New();
  certaintyReader->SetFileName( certaintyPath );
  
  // Setup the NormalizedConvolutionFilter
  typedef itk::NormalizedGaussianConvolutionImageFilter< ImageType > FilterType;
  FilterType::Pointer normConvFilter = FilterType::New();
  normConvFilter->SetInputImage( imageReader->GetOutput() );
  normConvFilter->SetInputCertainty( certaintyReader->GetOutput() );
  
  // Setup the writer
  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();

  // If we should mask the output we pass the divide filter through a mask filter
  // Setup the mask filter that applies the certainty as a mask
  typedef itk::MaskImageFilter< ImageType, ImageType, ImageType >
    MaskFilterType;
  MaskFilterType::Pointer maskFilter = MaskFilterType::New();
  if ( maskOutput ) {
    maskFilter->SetInput1( normConvFilter->GetOutput() );
    maskFilter->SetInput2( certaintyReader->GetOutput() );
    writer->SetInput( maskFilter->GetOutput() );
    // There is a strange issue. If the norm conv output is masked before writing,
    // then we have to do an explicit update before updating the writer. But if 
    // the output is not masked we dont have to do an explicit update.
    // try {
    //   maskFilter->Update();
    // }
    // catch ( itk::ExceptionObject &e ) {
    //   std::cerr << "Failed to process." << std::endl
    // 		<< "Image: " << imagePath << std::endl
    // 		<< "Certainty: " << certaintyPath << std::endl
    // 		<< "ExceptionObject: " << e << std::endl;
    //   return EXIT_FAILURE;
    // }
  }
  else {
    writer->SetInput( normConvFilter->GetOutput() );
  }

  // Base file name for output images
  std::string baseFileName = Path::join( outDirPath, prefix );
  
  // Do the convolution for each scale
  for (auto scale : scales ) {
    std::cout << "Processing scale " << scale << std::endl;
    normConvFilter->SetSigma(scale);
    
    // Create a filename
    std::string outFile = baseFileName + "scale_" + std::to_string( scale ) +
      OUT_FILE_TYPE;
    writer->SetFileName( outFile );

    try {
      writer->Update();
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to process." << std::endl
		<< "Image: " << imagePath << std::endl
		<< "Certainty: " << certaintyPath << std::endl
		<< "Scale: " << scale << std::endl
		<< "Base file name: " << baseFileName << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
