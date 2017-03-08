#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

#include "Eigen/Dense"

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkRegionOfInterestImageFilter.h"

#include "IO/ROIReader.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  typedef float PixelType;
  
  // Commandline parsing
  TCLAP::CmdLine cmd("Sample ROIs from an image.", ' ', VERSION);

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

  // We need a ROI file
  TCLAP::ValueArg<std::string> 
    roiArg("r", 
	   "roi-file", 
	   "Path to ROI file. If given the ROIs in this file will be used,"
	   "otherwise ROIs will be generated.",
	   false,
	   "",
	   "path", 
	   cmd);
  
  // We need a directory for storing the ROIs
  TCLAP::ValueArg<std::string> 
    outArg("o", 
	   "out", 
	   "Path to output",
	   true, 
	   "", 
	   "path", 
	   cmd);

  
  //
  // Optional arguments
  //
  
  TCLAP::ValueArg<bool>
    roiHasHeaderArg("R",
		    "roi-file-has-header",
		    "Flag indicating if the ROI file has a header",
		    false,
		    true,
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
  const std::string imagePath( imageArg.getValue() );
  const std::string roiPath( roiArg.getValue() );
  const std::string outPath( outArg.getValue() );

  // Optional
  const bool roiHasHeader( roiHasHeaderArg.getValue() );

  //// Commandline parsing is done ////

  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef itk::Image< PixelType, Dimension >  ImageType;

  typedef typename ImageType::SizeType SizeType;
  typedef typename ImageType::RegionType RegionType;


  // Setup the image reader
  typedef itk::ImageFileReader< ImageType > ImageReaderType;
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName( imagePath );

  // Read the roi specification
  std::vector< RegionType > rois;
  SizeType roiSize;
  typedef ROIReader< RegionType > ROIReaderType;
  try {
    rois = ROIReaderType::read( roiPath, roiHasHeader );
    std::cout << "Got " << rois.size() << " rois." << std::endl;
    
    // Check that all rois are the same size
    roiSize = rois.at(0).GetSize();
    for ( const auto& roi : rois ) {
      if ( roiSize != roi.GetSize() ) {
	std::cout << "ROI size differ: "
		  << roiSize << " | " << roi.GetSize() << std::endl;
	return EXIT_FAILURE;
      }
    }
    std::cout << "ROI size " << roiSize << "." << std::endl;
  }
  catch ( std::exception &e ) {
    std::cerr << "Error reading ROIs" << std::endl
	      << "roiPath: " << roiPath << std::endl
	      << "exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  // Setup the ROI extraction filter
  typedef itk::RegionOfInterestImageFilter< ImageType,
					    ImageType > ROIFilterType;
  ROIFilterType::Pointer roiFilter = ROIFilterType::New();
  roiFilter->SetInput( imageReader->GetOutput() );
  
  // The matrix that will store the samples
  // Each row represent a ROI
  // Each column is the value in the corresponding voxel
  typedef Eigen::Matrix< PixelType,
			 Eigen::Dynamic,
			 Eigen::Dynamic,
			 Eigen::RowMajor> MatrixType;
  MatrixType bag( rois.size(), roiSize[0]*roiSize[1]*roiSize[2] );

  typedef itk::ImageRegionConstIteratorWithIndex< ImageType >  IteratorType;
  // We process one ROI at a time
  for ( size_t i = 0; i < rois.size(); ++i ) {
      roiFilter->SetRegionOfInterest( rois[i] );
      try {
	roiFilter->Update();
      }
      catch ( itk::ExceptionObject &e ) {
	std::cerr << "Failed to update roiFilter." << std::endl       
		  << "ROI: " << rois[i] << std::endl
		  << "ExceptionObject: " << e << std::endl;
	return EXIT_FAILURE;
      }
      size_t j = 0;
      IteratorType iter( roiFilter->GetOutput(), roiFilter->GetOutput()->GetRequestedRegion() );
      for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
	bag(i,j++) = iter.Get();
      }
  }

  std::ofstream out( outPath );
  for ( typename MatrixType::Index i = 0; i < bag.rows(); ++i ) {
    for ( typename MatrixType::Index j = 0; j < bag.cols(); ++j ) {
      out << bag(i,j);
      if ( j + 1 < bag.cols() ) {
	out << ",";
      }
    }
    out << '\n';
  }
  if ( !out.good() ) {
    std::cerr << "Error writing bag to file" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
