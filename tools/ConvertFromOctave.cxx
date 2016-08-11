#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileWriter.h"
#include "ife/IO/OctaveReader.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  // Commandline parsing
  TCLAP::CmdLine cmd("Convert octave style ascii image to nifti.", ' ', VERSION);

  // We need a single mask
  TCLAP::ValueArg<std::string> 
    inFileArg("i", 
	      "infile", 
	      "Input file.",
	      true,
	      "",
	      "path", 
	      cmd);
  
  TCLAP::ValueArg<std::string> 
    outFileArg("o", 
	       "outfile", 
	       "Output file",
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
  std::string inPath( inFileArg.getValue() );
  std::string outPath( outFileArg.getValue() );
  //// Commandline parsing is done ////


  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef float PixelType;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef OctaveReader< PixelType, Dimension > ReaderType;    
  
  ReaderType reader( inPath );

  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetFileName( outPath );

  try {
    writer->SetInput( reader.GetOutput() );
    writer->Update();
  }
  catch ( std::invalid_argument &e ) {
    std::cerr << "Failed to read." << std::endl
	      << "inPath: " << inPath << std::endl
	      << "ExceptionObject: " << e.what() << std::endl;
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to write." << std::endl
	      << "outPath: " << outPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
