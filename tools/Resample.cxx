/* 
 */
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkTranslationTransform.h"

#include "ife/Util/Path.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  std::string usage = \
    "Resample a <source> image to match the pixel spacing of a <target> image. "	\
    "It is assumed that the images have the same coordinate system and are perfectly registered. " \
    "This is intended for resampling a segmentation mask from one reconstruction thickness to another reconstruction thickness.";
  
  // Commandline parsing
  TCLAP::CmdLine cmd(usage, ' ', VERSION);

  TCLAP::ValueArg<std::string>
    sourceArg("s", "source", "Path to source image.", true, "", "path", cmd);

  TCLAP::ValueArg<std::string>
    targetArg("t", "target", "Path to target image.", true, "", "path", cmd);

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
  const std::string sourcePath( sourceArg.getValue() );
  const std::string targetPath( targetArg.getValue() );
  const std::string outPath( outArg.getValue() );
  //// Commandline parsing is done //// 
   
  const unsigned int Dimension = 3;  
  typedef double PixelType;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::ImageFileReader< ImageType > ReaderType;
  
  // Read the input image
  ReaderType::Pointer sourceReader = ReaderType::New();
  ReaderType::Pointer targetReader = ReaderType::New();
  sourceReader->SetFileName( sourcePath );
  targetReader->SetFileName( targetPath );
  try {
    sourceReader->Update();
    targetReader->Update();
    std::cout << "== Source ==" << std::endl
	      << "Origin "    << sourceReader->GetOutput()->GetOrigin() << std::endl
	      << "Spacing "   << sourceReader->GetOutput()->GetSpacing() << std::endl
	      << "Direction " << sourceReader->GetOutput()->GetDirection() << std::endl
      	      << "Size "      << sourceReader->GetOutput()->GetLargestPossibleRegion().GetSize() << std::endl
	      << "== Target ==" << std::endl
	      << "Origin "    << targetReader->GetOutput()->GetOrigin() << std::endl
	      << "Spacing "   << targetReader->GetOutput()->GetSpacing() << std::endl
	      << "Direction " << targetReader->GetOutput()->GetDirection() << std::endl
      	      << "Size "      << targetReader->GetOutput()->GetLargestPossibleRegion().GetSize() << std::endl;
  }
  catch (itk::ExceptionObject& e ) {
    std::cerr << "Failed to read images" << std::endl
	      << "Source image path: " << sourcePath << std::endl
	      << "Target image path: " << targetPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  typedef itk::ResampleImageFilter<ImageType, ImageType> ResampleFilterType;
  ResampleFilterType::Pointer resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput( sourceReader->GetOutput() );

  typedef itk::TranslationTransform<double,Dimension> TransformType;
  const TransformType::Pointer transform = TransformType::New();
  TransformType::OutputVectorType translation;
  translation = sourceReader->GetOutput()->GetOrigin() - targetReader->GetOutput()->GetOrigin();
  std::cout << "Translation: " << translation << std::endl;
  transform->Translate( translation );	
  resampleFilter->SetTransform( transform );
  

  resampleFilter->SetOutputOrigin( targetReader->GetOutput()->GetOrigin() );
  resampleFilter->SetOutputSpacing( targetReader->GetOutput()->GetSpacing()  );
  resampleFilter->SetSize( targetReader->GetOutput()->GetLargestPossibleRegion().GetSize() );
  resampleFilter->UpdateLargestPossibleRegion();
  
  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetFileName( outPath );
  writer->SetInput( resampleFilter->GetOutput() );
  
  try {	
    resampleFilter->Update();
    std::cout << "Source image information after resampling" << std::endl
	      << "Origin "    << resampleFilter->GetOutput()->GetOrigin() << std::endl
	      << "Spacing "   << resampleFilter->GetOutput()->GetSpacing() << std::endl
	      << "Direction " << resampleFilter->GetOutput()->GetDirection() << std::endl
	      << "Size "      << resampleFilter->GetOutput()->GetLargestPossibleRegion().GetSize() << std::endl;
    writer->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to process." << std::endl
	      << "Source image: " << sourcePath << std::endl
	      << "Target image: " << targetPath << std::endl
	      << "Out path: " << outPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
    
