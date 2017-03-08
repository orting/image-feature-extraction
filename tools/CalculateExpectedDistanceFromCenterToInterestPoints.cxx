#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"

#include "Statistics/ExpectedDistanceFromCenterToInterestPoint.h"

const std::string VERSION("0.1");

int main( int argc, char* argv[] ) {
    // Commandline parsing
  TCLAP::CmdLine cmd("-", ' ', VERSION);

  //
  // Required arguments
  //
  
  // We need a single image
  TCLAP::ValueArg<std::string> 
    imageArg("p", 
	     "prob-image", 
	     "Path to probability image, expected to hold values in [0,1].",
	     true,
	     "",
	     "path", 
	     cmd);


  // We need a single image
  TCLAP::ValueArg<std::string> 
    maskArg("m", 
	    "mask", 
	    "Path to mask.",
	    true,
	    "",
	    "path", 
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
  //// Commandline parsing is done ////
  
  
  const unsigned int Dimension = 3;
  typedef itk::Image< double, Dimension > ImageType;
  typedef itk::Image< unsigned int, Dimension > MaskType;

  // Setup the reader
  typedef itk::ImageFileReader< ImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( imagePath );

  typedef itk::ImageFileReader< MaskType > MaskReaderType;
  MaskReaderType::Pointer maskReader = MaskReaderType::New();
  maskReader->SetFileName( maskPath );

  try {
    reader->Update();
    maskReader->Update();
  }
  catch (itk::ExceptionObject& e) {
    std::cout << "Failed to process." << std::endl
      	      << "Image: " << imagePath << std::endl
	      << "Mask: " << maskPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  
  double ed = expectedDistanceFromCenterToInterestPoint<MaskType, ImageType>( maskReader->GetOutput(), reader->GetOutput() );
  std::cout << ed << std::endl;

  return EXIT_SUCCESS;

}
