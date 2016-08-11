
// We need an image, a mask, a histogram specification and optionally a set of ROIs

#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

#include "Eigen/Dense"

#include "tclap/CmdLine.h"

#include "itkClampImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkVectorImage.h"

#include "ife/Filters/ImageToEmphysemaFeaturesFilter.h"
#include "ife/IO/IO.h"
#include "ife/IO/ROIReader.h"
#include "ife/ROI/RegionOfInterestGenerator.h"
#include "ife/Statistics/DenseHistogram.h"
#include "ife/Util/Path.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {
  typedef float PixelType;
  typedef unsigned short MaskPixelType;
  
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

  // We need a histogram specificaton
  TCLAP::ValueArg<std::string> 
    histArg("H", 
	    "histogram-spec", 
	    "Path to histogram specification.",
	    true,
	    "",
	    "path", 
	    cmd);

  
  // We need a directory for storing the ROIs
  TCLAP::ValueArg<std::string> 
    outDirArg("o", 
	      "outdir", 
	      "Path to output directory",
	      true, 
	      "", 
	      "path", 
	      cmd);
  
  //
  // Optional arguments
  //
  
  // We can use a ROI file or generate one
  TCLAP::ValueArg<std::string> 
    roiArg("r", 
	   "roi-file", 
	   "Path to ROI file. If given the ROIs in this file will be used,"
	   "otherwise ROIs will be generated.",
	   false,
	   "",
	   "path", 
	   cmd);

  TCLAP::ValueArg<bool>
    roiHasHeaderArg("R",
		    "roi-file-has-header",
		    "Flag indicating if the ROI file has a header",
		    false,
		    true,
		    "boolean",
		    cmd);
  
  TCLAP::ValueArg<std::string> 
    roiMaskArg("M", 
	       "roi-mask", 
	       "Path to ROI mask file. If ROIs are generated an optional mask "
	       "controlling the ROI generation can be used. If not given then "
	       "the image mask will be used.",
	       false,
	       "",
	       "path", 
	       cmd);

  TCLAP::ValueArg<MaskPixelType> 
    roiMaskValueArg("v", 
		    "roi-mask-value", 
		    "Value in the ROI mask that should be used for inclusion.",
		    false,
		    1,
		    "MaskPixelType", 
		    cmd);
  
  TCLAP::ValueArg<size_t> 
    numROIsArg("n", 
	       "num-rois", 
	       "Number of ROIs to sample",
	       false,
	       50,
	       "N>=2", 
	       cmd);
  
  TCLAP::ValueArg<size_t> 
    roiSizeXArg("x", 
		"roi-size-x", 
		"Size of ROI in x dimension",
		false,
		41,
		"N>=1", 
		cmd);

  TCLAP::ValueArg<size_t> 
    roiSizeYArg("y", 
		"roi-size-y", 
		"Size of ROI in y dimension",
		false,
		41,
		"N>=1", 
		cmd);

  TCLAP::ValueArg<size_t> 
    roiSizeZArg("z", 
		"roi-size-z", 
		"Size of ROI in z dimension",
		false,
		41,
		"N>=1", 
		cmd);

  
  // We can use a prefix to generate filenames
  TCLAP::ValueArg<std::string> 
    prefixArg("p", 
	      "prefix", 
	      "Prefix to use for output filenames",
	      false, 
	      "", 
	      "string", 
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
  const std::string histPath( histArg.getValue() );
  const std::string outDirPath( outDirArg.getValue() );

  // Optional
  const std::string roiPath( roiArg.getValue() );
  const bool roiHasHeader( roiHasHeaderArg.getValue() );
  const std::string roiMaskPath( roiMaskArg.getValue() );
  const MaskPixelType roiMaskValue( roiMaskValueArg.getValue() );
  const size_t numROIs( numROIsArg.getValue() );
  const size_t roiSizeX = roiSizeXArg.getValue();
  const size_t roiSizeY = roiSizeYArg.getValue();
  const size_t roiSizeZ = roiSizeZArg.getValue();
  const std::string prefix( prefixArg.getValue() );
  //// Commandline parsing is done ////


  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::Image< MaskPixelType, Dimension >  MaskImageType;

  typedef typename ImageType::SizeType SizeType;
  typedef typename ImageType::RegionType RegionType;


  // Setup the readers
  typedef itk::ImageFileReader< ImageType > ImageReaderType;
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName( imagePath );

  typedef itk::ImageFileReader< MaskImageType > MaskReaderType;
  MaskReaderType::Pointer maskReader = MaskReaderType::New();
  maskReader->SetFileName( maskPath );

  MaskReaderType::Pointer roiMaskReader = MaskReaderType::New();
  roiMaskReader->SetFileName( roiMaskPath );

  // Setup a filter that ensures the mask is binary.
  // Consider if this is still necesary
  typedef itk::ClampImageFilter< MaskImageType,
				 MaskImageType > ClampFilterType;
  ClampFilterType::Pointer clampFilter = ClampFilterType::New();
  clampFilter->InPlaceOff();
  clampFilter->SetBounds(0, 1);
  clampFilter->SetInput( maskReader->GetOutput() );


  // Setup a filter that can extract the requested region from the ROI mask
  typedef itk::BinaryThresholdImageFilter<
    MaskImageType,
    MaskImageType > RoiThresholdFilterType;
  RoiThresholdFilterType::Pointer roiThresholdFilter = RoiThresholdFilterType::New();
  roiThresholdFilter->SetLowerThreshold( roiMaskValue );
  roiThresholdFilter->SetUpperThreshold( roiMaskValue );
  roiThresholdFilter->SetInsideValue( 1 );
  roiThresholdFilter->SetOutsideValue( 0 );
  roiThresholdFilter->SetInput( roiMaskReader->GetOutput() );  
  

  // If we have a ROI specification file we use that, otherwise we
  // generate a set of ROIs
  std::vector< RegionType > rois;
  if ( roiPath.empty() ) {
    // Generate the ROIs. If we have a ROI mask we use that, otherwise we use the
    // image mask
    typedef itk::RegionOfInterestGenerator< MaskImageType > ROIGeneratorType;    
    ROIGeneratorType roiGenerator( clampFilter->GetOutput() );

    if ( !roiMaskPath.empty() ) {
      std::cout << "Using ROI mask." << std::endl;
      roiGenerator.setMask( roiThresholdFilter->GetOutput() );
    }
    
    SizeType roiSize{ roiSizeX, roiSizeY, roiSizeZ };
    try {
      rois = roiGenerator.generate( numROIs, roiSize );
      // We should store the generated ROIs
      std::string roiFileName = prefix + ".ROIInfo";
      std::string roiOutPath( Path::join( outDirPath, roiFileName ) );
      std::ofstream out( roiOutPath );
      for ( auto roi : rois ) {
	out << roi.GetIndex() << roi.GetSize() << '\n';
      }
      if ( !out.good() ) {
	std::cerr << "Error writing ROI info file" << std::endl;
	return EXIT_FAILURE;
      }
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to generate ROIs." << std::endl       
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }
  else {
    // Read the roi specification
    typedef ROIReader< RegionType > ROIReaderType;
    try {
      rois = ROIReaderType::read( roiPath, roiHasHeader );
      std::cout << "Got " << rois.size() << " rois." << std::endl;
    }
    catch ( std::exception &e ) {
      std::cerr << "Error reading ROIs" << std::endl
		<< "roiPath: " << roiPath << std::endl
		<< "exception: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  }
  
  
  // Setup the histogram containers
  typedef DenseHistogram< PixelType > HistogramType;
  std::vector< HistogramType > histograms;

  // Read the histogram edge spec
  std::ifstream isHist( histPath );
  if ( !isHist.good() ) {
    std::cerr << "Could not read histogram file '" << histPath << "'" << std::endl;
    return EXIT_FAILURE;
  }
  
  while ( isHist.good() ) {
    std::string line;
    std::getline( isHist, line );
    if ( line.empty() ) {
      std::cout << "Empty line. Breaking" << std::endl;
      break;
    }
    if ( line[0] == '#' ) {
      // Skip it
      std::cout << "Skipping a line" << std::endl;
      continue;
    }
    std::stringstream ss( line );
    std::vector< PixelType > edges;
    readTextSequence< PixelType, char >( ss, std::back_inserter(edges) );
    histograms.emplace_back( HistogramType( edges.begin(), edges.end() ));
  }

  if ( histograms.size() != 1 ) {
    std::cerr << "[ERROR] Expected exactly one histogram in histogram specification. Got "
	      << histograms.size() << std::endl;
      return EXIT_FAILURE;
  }
    
  // Setup the ROI extraction filter
  // We need one for the intensity and one for the mask
  typedef itk::RegionOfInterestImageFilter< ImageType, ImageType > ROIFilterType;
  ROIFilterType::Pointer roiFilter = ROIFilterType::New();
  roiFilter->SetInput( imageReader->GetOutput() );

  typedef itk::RegionOfInterestImageFilter< MaskImageType, MaskImageType > MaskROIFilterType;
  MaskROIFilterType::Pointer maskROIFilter = MaskROIFilterType::New();
  maskROIFilter->SetInput( clampFilter->GetOutput() );
  
  // We need to iterate over the mask to get the pixels to sample
  typedef itk::ImageRegionConstIteratorWithIndex< MaskImageType > MaskIteratorType;
  
  // The matrix that will store the bag
  // Each row represent a bag.
  // Each column is a bin in one of the histograms
  typedef Eigen::Matrix< PixelType,
			 Eigen::Dynamic,
			 Eigen::Dynamic,
			 Eigen::RowMajor > MatrixType;
  MatrixType bag( rois.size(), histograms[0].getNumberOfBins() );

  // Now we can run the pipeline
  // We process one ROI at a time
  for ( size_t j = 0; j < rois.size(); ++j ) {
    roiFilter->SetRegionOfInterest( rois[j] );
    maskROIFilter->SetRegionOfInterest( rois[j] );
    try {
      roiFilter->Update();
      maskROIFilter->Update();
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to update maskROIFilter." << std::endl       
		<< "ROI: " << rois[j] << std::endl
		<< "Image region: " << imageReader->GetOutput()->GetLargestPossibleRegion() << std::endl
		<< "Clamp filter region: " << clampFilter->GetOutput()->GetLargestPossibleRegion() << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
      
    // Setup the mask iterator
    MaskIteratorType
      maskIter( maskROIFilter->GetOutput(),
		maskROIFilter->GetOutput()->GetRequestedRegion() );

    ImageType::Pointer roi( roiFilter->GetOutput() );
    for ( maskIter.GoToBegin(); !maskIter.IsAtEnd(); ++maskIter ) {
      if ( maskIter.Get() ) {
	histograms[0].insert( roi->GetPixel( maskIter.GetIndex() ) );
      }
    }
      
    // Now we add the histograms to the bag at row j.
    // We need to use the column range
    auto frequencies = histograms[0].getFrequencies();
    histograms[0].resetCounts();
    for ( size_t l = 0; l < frequencies.size(); ++l ) {
      bag(j, l) = frequencies[l];
    }
  }


  std::string fileName = prefix + ".bag";
  std::string outPath( Path::join( outDirPath, fileName ) );
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
    std::cerr << "Error writing histogram to file" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
