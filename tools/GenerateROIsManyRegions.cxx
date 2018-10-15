/* 
   Generate cubic ROIs at random such that the center of the ROIs lies inside
   the mask.
*/
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkBinaryThresholdImageFilter.h"

#include "ROI/RegionOfInterestGenerator.h"

const std::string VERSION("0.2");

int main(int argc, char *argv[]) {
  // Commandline parsing
  TCLAP::CmdLine cmd("Generate 3D ROIs.", ' ', VERSION);

  // We need a single mask
  TCLAP::ValueArg<std::string> 
    maskArg("m", 
	    "mask", 
	    "Path to mask.",
	    true,
	    "",
	    "path", 
	    cmd);
  
  // We need a directory for storing the ROI info
  TCLAP::ValueArg<std::string> 
    outFileArg("o", 
	      "outfile", 
	      "Basepath for output files. <Maskvalue>.ROIInfo is appended",
	      true, 
	      "", 
	      "path", 
	      cmd);
  
  // We need a number of ROIs to sample
  TCLAP::ValueArg<unsigned int> 
    numROIsArg("n", 
	       "num-rois", 
	       "Number of ROIs to sample",
	       false, 
	       50, 
	       "unsigned int", 
	       cmd);

  // We need the size of the ROIs
  TCLAP::ValueArg<size_t> 
    roiSizeXArg("x", 
		"roi-size-x", 
		"Size of ROI in x dimension",
		false,
		53,
		"N>=1", 
		cmd);

    TCLAP::ValueArg<size_t> 
    roiSizeYArg("y", 
		"roi-size-y", 
		"Size of ROI in y dimension",
		false,
		53,
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


  // Indicate which values of the mask indicates foreground
  TCLAP::MultiArg<unsigned int> 
    maskValueArgs("M", 
		  "mask-value", 
		  "The value of the mask that is inside the region of interest.",
		  false, 
		  "unsigned int", 
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
  const std::string maskPath( maskArg.getValue() );
  const std::string outFilePath( outFileArg.getValue() );
  const unsigned int numROIs( numROIsArg.getValue() );
  const size_t roiSizeX = roiSizeXArg.getValue();
  const size_t roiSizeY = roiSizeYArg.getValue();
  const size_t roiSizeZ = roiSizeZArg.getValue();
  const std::vector<unsigned int> maskValues( maskValueArgs.getValue() );

  // Check parameters
  assert( numROIs > 0 );
  assert( roiSizeX > 0 );
  assert( roiSizeY > 0 );
  assert( roiSizeZ > 0 );
  
  //// Commandline parsing is done ////


  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef unsigned char MaskPixelType;
  typedef itk::Image< MaskPixelType, Dimension >  MaskImageType;
  typedef itk::ImageFileReader< MaskImageType > MaskReaderType;
    
  // Setup the reader
  MaskReaderType::Pointer maskReader = MaskReaderType::New();
  maskReader->SetFileName( maskPath );

  try {
    maskReader->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to Update mask reader." << std::endl
	      << "Mask: " << maskPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  // Setup a filter that can extract the requested region from the mask
  typedef itk::BinaryThresholdImageFilter<
    MaskImageType,
    MaskImageType > RoiThresholdFilterType;
  RoiThresholdFilterType::Pointer roiThresholdFilter = RoiThresholdFilterType::New();
  roiThresholdFilter->SetInsideValue( 1 );
  roiThresholdFilter->SetOutsideValue( 0 );
  roiThresholdFilter->SetInput( maskReader->GetOutput() ); 


  // Setup the ROI generator 
  typedef itk::RegionOfInterestGenerator< MaskImageType > ROIGeneratorType;    
  ROIGeneratorType roiGenerator( roiThresholdFilter->GetOutput() );
  MaskImageType::SizeType roiSize{ {roiSizeX, roiSizeY, roiSizeZ} };

  for ( const auto maskValue : maskValues ) {
    roiThresholdFilter->SetLowerThreshold( maskValue );
    roiThresholdFilter->SetUpperThreshold( maskValue );

    try {
      auto rois = roiGenerator.generate( numROIs, roiSize );

      // Store the generated ROIs
      std::ofstream out( outFilePath + std::to_string(maskValue) + ".ROIInfo" );
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

  return EXIT_SUCCESS;
}
