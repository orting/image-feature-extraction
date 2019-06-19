/* 
   Extract slices from an image
 */
#include <numeric>
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkMaskImageFilter.h"
#include "itkBinaryImageToShapeLabelMapFilter.h"
#include "itkExtractImageFilter.h"
#include "itkFlipImageFilter.h"

#include "ife/Util/Path.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  // Commandline parsing
  TCLAP::CmdLine cmd("Extract slices from an image.", ' ', VERSION);

  // We need a single image
  TCLAP::ValueArg<std::string> 
    imageArg("i", "image", "Path to image.", true, "", "path", cmd);

  TCLAP::ValueArg<std::string> 
    outArg("o", "out", "Base path to output files", true, "", "path", cmd);


  TCLAP::ValueArg<std::string>
    imageTypeArg("t", "type", "Image type suffix", false, "nii.gz", "suffix", cmd);
  
  // If we get a mask we can ensure that slices are within the mask
  TCLAP::ValueArg<std::string> 
    maskArg("m", "mask", "Path to mask. Must match image dimensions.", false, "", "path", cmd);

  // We can pick slices by index
  TCLAP::MultiArg<unsigned int>
    sliceIndexArg("s", "slice-index", "Index of slices to extract", false, "unsigned int", cmd);

  // or by relative location
  TCLAP::MultiArg<double>
    sliceLocationArg("l", "slice-location", "Location of slices to extract", false, "[0,1]", cmd);

  TCLAP::ValueArg<unsigned int>
    sliceWindowArg("w", "slice-window", "Size of window around slice locations/indices that should be extracted. This is used to extract slices neighboring those defined with slice-index and slice-location.", false, 0, "unsigned int", cmd);

  TCLAP::ValueArg<unsigned int>
    sliceStrideArg("d", "slice-stride", "Stride to use when selecting slices in the slice-window. Ignored if slice window <= 1. Example usage: '--slice-window 10 --slice-stride 5' will extract the 5th preceding and following slices ", false, 0,"unsigned int", cmd);
  
  // Axis 0 == Sagittal
  //      1 == Coronal
  //      2 == Axial
  TCLAP::ValueArg<unsigned int>
    axisIndexArg("a", "axis-index", "Index of axis to extract", false, 2, "0:sagittal, 1:coronal, 2:axial", cmd);

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
  const std::string outBasePath( outArg.getValue() );

  const std::string suffix( imageTypeArg.getValue() );  
  const std::string maskPath( maskArg.getValue() );
  std::vector<unsigned int> indices( sliceIndexArg.getValue() );
  const std::vector<double> locations( sliceLocationArg.getValue() );
  const unsigned int sliceWindow( sliceWindowArg.getValue() );
  const unsigned int sliceStride( sliceStrideArg.getValue() );
  const unsigned int axisIndex( axisIndexArg.getValue() );
  //// Commandline parsing is done //// 
  
  
  const unsigned int Dimension = 3;

  if ( axisIndex >= Dimension ) {
    std::cerr << "Axis index must be less than the number of dimensions" << std::endl;
    return EXIT_FAILURE;
  }

  typedef double PixelType;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::Image< PixelType, Dimension-1 >  SliceType;
  typedef typename ImageType::RegionType RegionType;
  typedef itk::ImageFileReader< ImageType > ReaderType;

  // Read the input image
  ReaderType::Pointer imageReader = ReaderType::New();
  imageReader->SetFileName( imagePath );
  try {
    imageReader->Update();
    std::cout << "Origin "    << imageReader->GetOutput()->GetOrigin() << std::endl
	      << "Spacing "   << imageReader->GetOutput()->GetSpacing() << std::endl
	      << "Direction " << imageReader->GetOutput()->GetDirection() << std::endl;
  }
  catch (itk::ExceptionObject& e ) {
    std::cerr << "Failed to read image." << std::endl
	      << "Image path: " << imagePath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  RegionType region = imageReader->GetOutput()->GetLargestPossibleRegion();
  std::cout << "Region " << region << std::endl;
  
  if ( !maskPath.empty() ) {
    // Read the mask and use it to find a bounding box that we use for slice extraction
    typedef bool MaskPixelType;
    typedef itk::Image< MaskPixelType, Dimension >  MaskType;
    typedef itk::ImageFileReader< MaskType > MaskReaderType;
    MaskReaderType::Pointer maskReader = MaskReaderType::New();
    maskReader->SetFileName( maskPath );

    typedef itk::BinaryImageToShapeLabelMapFilter<MaskType> LabelFilterType;
    LabelFilterType::Pointer labelFilter = LabelFilterType::New();
    labelFilter->SetInput( maskReader->GetOutput() );

    try {
      labelFilter->Update();
      unsigned int numberOfComponents = labelFilter->GetOutput()->GetNumberOfLabelObjects();
      if ( numberOfComponents == 0 ) {
	std::cerr << "At least one connected components expected" << std::endl
		  << "Got 0" << std::endl;
	return EXIT_FAILURE;
      }
      else {
	region = labelFilter->GetOutput()->GetNthLabelObject(0)->GetBoundingBox();
	// Find bounding box for all components
	for ( unsigned int i = 1; i < numberOfComponents; ++i ) {
	  auto nextRegion = labelFilter->GetOutput()->GetNthLabelObject(i)->GetBoundingBox(); 
	  if ( ! region.IsInside( nextRegion ) ) {
	    auto csize = region.GetSize();
	    auto cindex = region.GetIndex();
	    auto nsize = nextRegion.GetSize();
	    auto nindex = nextRegion.GetIndex();
	    auto index = cindex;
	    auto size = csize;
	    for ( size_t j = 0; j < Dimension; ++j ) {
	      index[j] = std::min(cindex[j], nindex[j]);
	      size[j] = std::max(cindex[j] + csize[j], nindex[j] + nsize[j]) - index[j];
	    }
	    region.SetSize(size);
	    region.SetIndex(index);
	  }
	}
      }
      std::cout << "Region " << region << std::endl;
    }
    catch (itk::ExceptionObject& e ) {
      std::cerr << "Failed to read mask." << std::endl
		<< "Mask path: " << maskPath << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }    

  if ( !locations.empty() ) {
    // If we get fractional locations we convert them to indices
    double maxIndex = static_cast<double>(region.GetSize()[axisIndex]) - 1;
    for ( double location : locations ) {
      if ( location >= 0 && location <= 1 ) {
	indices.push_back(static_cast<unsigned int>( std::round(location*maxIndex) ));
      }
    }
  }

  if ( indices.empty() ) {
    // If neither indices nor locations are specified we take all slices
    indices.resize(region.GetSize()[axisIndex]);
    std::iota(indices.begin(), indices.end(), 0 );
  }  
  else {
    // Add slices from window
    const unsigned int maxOffset = sliceWindow / 2;
    const unsigned int outOfBoundsIndex = region.GetSize()[axisIndex];
    if ( maxOffset > 0 ) {
      std::vector<unsigned int> selectedIndices( indices );
      for ( unsigned int idx : selectedIndices ) {
	for ( unsigned int offset = sliceStride; offset <= maxOffset; offset += sliceStride) {
	  unsigned int before = idx - offset;
	  unsigned int after = idx + offset;
	  if ( before < idx ) {
	    indices.push_back( before );
	  }
	  if ( after > idx && after < outOfBoundsIndex ) {
	    indices.push_back( after );
	  }
	}
      }
    }

    
    // Remove duplicate indices and verify that they are within bounds
    std::sort( indices.begin(), indices.end() );
    auto last = std::unique( indices.begin(), indices.end() );
    indices.erase(last, indices.end() );
    if ( indices.back() >= outOfBoundsIndex ) {
      std::cerr << "Slice indices are outside bounds" << std::endl
		<< "Largest requested index is " << indices.back() << std::endl
		<< "Largest possible index is " << region.GetSize()[axisIndex] << std::endl;
      return EXIT_FAILURE;
    }
  }  
  
  typedef itk::ExtractImageFilter<ImageType,SliceType> ExtractFilterType;
  ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
  extractFilter->SetDirectionCollapseToIdentity();
  extractFilter->SetInput( imageReader->GetOutput() );

  // If we are extracting axis 0 or 1 we need to flip the axis. This can be done
  // by changing the direction information of the image
  //
  // However, if we only change direction information the flip will be ignored
  // if we save the image in a format that does not store image direction
  // information, such as png.
  // Instead we can flip the slice along the second axis which is equivalent to
  // applying the transformation [[1 0],[0,-1]]
  typedef itk::FlipImageFilter<SliceType> DirectionFilterType;
  DirectionFilterType::Pointer directionFilter = DirectionFilterType::New();
  directionFilter->SetInput( extractFilter->GetOutput() );
  itk::FixedArray<bool,2> flipAxes;
  flipAxes[0] = false;
  flipAxes[1] = axisIndex != 2;
  directionFilter->SetFlipAxes(flipAxes);
   
  typedef itk::ImageFileWriter< SliceType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();

  // We set the size of the axis we collapse to zero
  RegionType::SizeType size = region.GetSize();
  size[axisIndex] = 0;
  
  for ( unsigned int sliceIndex : indices ) {
    RegionType::IndexType index = region.GetIndex();
    index[axisIndex] += sliceIndex;
    extractFilter->SetExtractionRegion( RegionType(index, size) );

    std::string out =
      outBasePath
      + "axis-" + std::to_string(axisIndex)
      + "_absslice-" + std::to_string( index[axisIndex] )
      + "_relslice-" + std::to_string( sliceIndex )
      + "." + suffix;
    writer->SetFileName( out );
    writer->SetInput( directionFilter->GetOutput() );
    
    try {
      directionFilter->Update();
      writer->Update();
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to process." << std::endl
		<< "Image: " << imagePath << std::endl
		<< "Mask: " << maskPath << std::endl
		<< "Out: " << out << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
    
