/* 
   Rescale intensities and conver to unsigned 8 bit
*/
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkIntensityWindowingImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkTranslationTransform.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkNearestNeighborExtrapolateImageFunction.h"
#include "itkMaskImageFilter.h"

#include "ife/Util/Path.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  // Commandline parsing
  TCLAP::CmdLine cmd("Rescale intensity and convert to unsigned 8-bit.", ' ', VERSION);

  TCLAP::ValueArg<std::string> 
    imageArg("i", "image", "Path to image.", true, "", "path", cmd);

  TCLAP::ValueArg<std::string> 
    outArg("o", "out", "Output path", true, "", "path", cmd);

  TCLAP::ValueArg<std::string> 
    maskArg("m", "mask", "Path to mask.", false, "", "path", cmd);

  TCLAP::ValueArg<double>
    widthArg("w", "width", "Window width", false, 1500, "unsigned int", cmd);

  TCLAP::ValueArg<double>
    levelArg("l", "level", "Window level", false, -500, "int", cmd);

  TCLAP::ValueArg<unsigned int>
    splineOrderArg("b", "spline-order", "B-spline order for interpolation", false, 3, "[0,1,2,3,4,5]", cmd);
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
  const std::string outPath( outArg.getValue() );
  const std::string maskPath( maskArg.getValue() );
  const double width( widthArg.getValue() );
  const double level( levelArg.getValue() );
  const unsigned int splineOrder( splineOrderArg.getValue() );
  //// Commandline parsing is done //// 

  if ( splineOrder > 5 ) {
    std::cerr << "Spline order must be <= 5" << std::endl;
    return EXIT_FAILURE;
  }
  
  const unsigned int Dimension = 2;
  
  typedef double InPixelType;
  typedef unsigned char OutPixelType;
  typedef double MaskPixelType;
  typedef itk::Image< InPixelType, Dimension >  InImageType;
  typedef itk::Image< OutPixelType, Dimension >  OutImageType;
  typedef itk::Image< MaskPixelType, Dimension >  MaskImageType;

  
  typedef itk::ImageFileReader<InImageType> ReaderType;
  ReaderType::Pointer imageReader = ReaderType::New();
  imageReader->SetFileName( imagePath );

  try {
    imageReader->Update();
  }
  catch (itk::ExceptionObject &e) {
    std::cerr << "Failed reading image." << std::endl
	      << "Path: " << imagePath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }


  typedef itk::ImageFileReader<MaskImageType> MaskReaderType;
  MaskReaderType::Pointer maskReader = MaskReaderType::New();

  if ( !maskPath.empty() ) {
    maskReader->SetFileName( imagePath );
    try {
      maskReader->Update();
    }
    catch (itk::ExceptionObject &e) {
      std::cerr << "Failed reading masp." << std::endl
		<< "Path: " << maskPath << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }

  
  // If we have anisotropic spacing in the image, we resample to get isotropic spacing
  // Use largest spacing as common spacing
  InImageType::SpacingType newSpacing;
  for ( size_t i = 0; i < Dimension; ++i ) {
    newSpacing[i] = 0.25;
  }

  // If we change the spacing we should also change the size.
  InImageType::SpacingType spacing = imageReader->GetOutput()->GetSpacing();
  InImageType::SizeType size = imageReader->GetOutput()->GetLargestPossibleRegion().GetSize();
  InImageType::SizeType newSize;
  for ( size_t i = 0; i < Dimension; ++i ) {
    newSize[i] = std::ceil( (static_cast<double>(size[i]) * spacing[i]) / newSpacing[i] );
  }

  // Setup translation to account for non-zero origin
  typedef itk::TranslationTransform<double,Dimension> TransformType;
  const TransformType::Pointer transform = TransformType::New();
  TransformType::OutputVectorType translation;
  InImageType::PointType origin = imageReader->GetOutput()->GetOrigin();
  for ( size_t i = 0; i < Dimension; ++i ) {
    translation[i] = origin[i];
  }
  transform->Translate( translation );

  // We use B-spline interpolation to sample new pixel values
  typedef itk::BSplineInterpolateImageFunction< InImageType > InterpolatorType;
  InterpolatorType::Pointer interpolator = InterpolatorType::New();
  interpolator->SetSplineOrder( splineOrder );

  // We use nearest neighbor interpolation to extrapolate values at the boundary
  typedef itk::NearestNeighborExtrapolateImageFunction< InImageType, InPixelType > ExtrapolatorType;
  ExtrapolatorType::Pointer extrapolator = ExtrapolatorType::New();  
  
  typedef itk::ResampleImageFilter<InImageType, InImageType> ResampleFilterType;
  ResampleFilterType::Pointer resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput( imageReader->GetOutput() );
  resampleFilter->SetTransform( transform );
  resampleFilter->SetInterpolator( interpolator );
  resampleFilter->SetExtrapolator( extrapolator );
  resampleFilter->SetOutputSpacing( newSpacing );
  resampleFilter->SetSize( newSize );

  try {
    std::cerr << "Resampling image." << std::endl
	      << "Origin:      	   " << origin << std::endl
	      << "Translation: 	   " << translation << std::endl
	      << "OldSize:     	   " << size << std::endl
	      << "Size:        	   " << newSize << std::endl
	      << "Old spacing: 	   " << spacing << std::endl
	      << "Spacing:     	   " << newSpacing << std::endl;
    resampleFilter->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to resample." << std::endl
	      << "Origin:      	   " << origin << std::endl
	      << "Translation: 	   " << translation << std::endl
	      << "OldSize:     	   " << size << std::endl
	      << "Size:        	   " << newSize << std::endl
	      << "Old spacing: 	   " << spacing << std::endl
	      << "Spacing:     	   " << newSpacing << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  
  // Keep direction
  //	resampleFilter->SetOutputDirection( extractFilter->GetOutput()->GetDirection() );
  
  typedef itk::IntensityWindowingImageFilter<InImageType,OutImageType> WindowFilterType;
  WindowFilterType::Pointer windowFilter = WindowFilterType::New();
  windowFilter->SetInput( resampleFilter->GetOutput() );

  try {
    windowFilter->SetWindowMinimum( level - width/2 );
    windowFilter->SetWindowMaximum( level + width/2 );
    windowFilter->SetOutputMinimum( 0 );
    windowFilter->SetOutputMaximum( 255 );
    windowFilter->Update();
  }
  catch (itk::ExceptionObject& e ) {
    std::cerr << "Failed to window image." << std::endl
	      << "Level:           " << level << std::endl
	      << "Width:           " << width << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }  
    
  typedef itk::ImageFileWriter< OutImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetFileName( outPath );
  
  if ( maskPath.empty() ) {
    writer->SetInput( windowFilter->GetOutput() );
  }
  else {
    // This is not really that nice, when all we want is to set the background to zero
    // If we have anisotropic spacing in the image, we resample to get isotropic spacing
    // Use largest spacing as common spacing
    MaskImageType::SpacingType newSpacing;
    for ( size_t i = 0; i < Dimension; ++i ) {
      newSpacing[i] = 0.25;
    }

    // If we change the spacing we should also change the size.
    MaskImageType::SpacingType spacing = maskReader->GetOutput()->GetSpacing();
    MaskImageType::SizeType size = maskReader->GetOutput()->GetLargestPossibleRegion().GetSize();
    MaskImageType::SizeType newSize;
    for ( size_t i = 0; i < Dimension; ++i ) {
      newSize[i] = std::ceil( (static_cast<double>(size[i]) * spacing[i]) / newSpacing[i] );
    }

    // Setup translation to account for non-zero origin
    typedef itk::TranslationTransform<double,Dimension> TransformType;
    const TransformType::Pointer transform = TransformType::New();
    TransformType::OutputVectorType translation;
    MaskImageType::PointType origin = imageReader->GetOutput()->GetOrigin();
    for ( size_t i = 0; i < Dimension; ++i ) {
      translation[i] = origin[i];
    }
    transform->Translate( translation );

    // We use nearest neighbour interpolator to ensure we get a binary mask
    typedef itk::NearestNeighborInterpolateImageFunction< MaskImageType > InterpolatorType;
    InterpolatorType::Pointer interpolator = InterpolatorType::New();

    // We use nearest neighbor interpolation to extrapolate values at the boundary
    typedef itk::NearestNeighborExtrapolateImageFunction< MaskImageType, MaskPixelType > ExtrapolatorType;
    ExtrapolatorType::Pointer extrapolator = ExtrapolatorType::New();  
  
    typedef itk::ResampleImageFilter<MaskImageType, MaskImageType> ResampleFilterType;
    ResampleFilterType::Pointer maskResampleFilter = ResampleFilterType::New();
    maskResampleFilter->SetInput( maskReader->GetOutput() );
    maskResampleFilter->SetTransform( transform );
    maskResampleFilter->SetInterpolator( interpolator );
    maskResampleFilter->SetExtrapolator( extrapolator );
    maskResampleFilter->SetOutputSpacing( newSpacing );
    maskResampleFilter->SetSize( newSize );

    try {
      std::cerr << "Resampling mask." << std::endl
		<< "Origin:      	   " << origin << std::endl
		<< "Translation: 	   " << translation << std::endl
		<< "OldSize:     	   " << size << std::endl
		<< "Size:        	   " << newSize << std::endl
		<< "Old spacing: 	   " << spacing << std::endl
		<< "Spacing:     	   " << newSpacing << std::endl;
      maskResampleFilter->Update();
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to resample mask." << std::endl
		<< "Origin:      	   " << origin << std::endl
		<< "Translation: 	   " << translation << std::endl
		<< "OldSize:     	   " << size << std::endl
		<< "Size:        	   " << newSize << std::endl
		<< "Old spacing: 	   " << spacing << std::endl
		<< "Spacing:     	   " << newSpacing << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
    
    typedef itk::MaskImageFilter<OutImageType, MaskImageType> MaskFilterType;
    MaskFilterType::Pointer maskFilter = MaskFilterType::New();
    maskFilter->SetInput( windowFilter->GetOutput() );
    maskFilter->SetMaskImage( maskResampleFilter->GetOutput() );
    maskFilter->SetOutsideValue( 0 );    
    writer->SetInput( maskFilter->GetOutput() );

    try { 
      maskFilter->Update();
    }
    catch (itk::ExceptionObject& e) {
      std::cerr << "Failed to mask image." << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }

  
  try {
    writer->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to save image." << std::endl
	      << "Out:             " << outPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
    
