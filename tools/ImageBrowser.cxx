
#include <iostream>
#include <set>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageRegionIterator.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkAndImageFilter.h"

#include "ROI/RegionOfInterestGenerator.h"

#include "Statistics/DenseHistogram.h"

const std::string VERSION("0.1");


template<typename PixelType, int Dimension >
void EstimateROICoverageFromImage( typename itk::Image< PixelType, Dimension >::Pointer image ) {
  typedef itk::Image< PixelType, Dimension > ImageType;
  typedef itk::Image<bool, Dimension> MaskType;
  typedef itk::ImageRegionIteratorWithIndex< MaskType > IteratorType;

  unsigned int x,y,z;
  PixelType low,high;
  std::cout << "ROI size (x y z): "; std::cin >> x >> y >> z;
  std::cout << "Threshold for inclusion (low high): "; std::cin >> low >> high;
  
  // Setup a filter that can extract the requested region from the mask
  typedef itk::BinaryThresholdImageFilter< ImageType,  MaskType > ThresholdFilterType;
  typename ThresholdFilterType::Pointer thresholdFilter = ThresholdFilterType::New();
  thresholdFilter->SetLowerThreshold( low );
  thresholdFilter->SetUpperThreshold( high );
  thresholdFilter->SetInsideValue( true );
  thresholdFilter->SetOutsideValue( false );
  thresholdFilter->SetInput( image );
  thresholdFilter->Update();
  auto mask = thresholdFilter->GetOutput();

  itk::RegionOfInterestGenerator< MaskType > generator( thresholdFilter->GetOutput() );
  
  typename MaskType::Pointer visited = MaskType::New();
  visited->SetOrigin( image->GetOrigin() );
  visited->SetSpacing( image->GetSpacing() );
  visited->SetRegions( image->GetRequestedRegion() );
  visited->Allocate( true );


  // typedef itk::AndImageFilter< MaskType,  MaskType, MaskType > AndFilterType;
  // typename AndFilterType::Pointer andFilter = AndFilterType::New();
  // andFilter->SetInput(0, thresholdFilter->GetOutput() );
  // andFilter->SetInput(1, visited );
  
  // Count the number of voxels in the region
  IteratorType iter( mask, mask->GetRequestedRegion() );
  unsigned int regionSize = 0;
  for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
    if ( iter.Get() ) {
      ++regionSize;
    }
  }

  
  // Try different numbers of ROIs
  std::vector< unsigned int > nSamplesPerRound{ 10, 10, 10, 10, 10, 50, 100, 100, 100, 100, 500, 1000 };
  unsigned int nROIs = 0;
  for ( auto nSamples : nSamplesPerRound ) {
    nROIs += nSamples;

    // Generate ROIs and mark voxels in the ROIs as visited
    for ( const auto& roi : generator.generate( nSamples, {x,y,z} ) ) {
      IteratorType roiIter( visited, roi );
      for ( roiIter.GoToBegin(); !roiIter.IsAtEnd(); ++roiIter ) {
	roiIter.Set( true );
      }
    }

    // Count the number of voxels in overlap between region and rois
    unsigned int hits = 0;
    unsigned int nVisited = 0;
    IteratorType visitedIter( visited, visited->GetRequestedRegion() );
    for ( visitedIter.GoToBegin(); !visitedIter.IsAtEnd(); ++visitedIter ) {
      if ( visitedIter.Get()) {
	++nVisited;
	if ( mask->GetPixel(visitedIter.GetIndex()) ) {
	  ++hits;
	}
      }
    }
    std::cout << "Visited " << nVisited << " voxels. "
	      << nROIs << " ROIs overlap " << hits << "/" << regionSize << " = "
  	      << static_cast<double>(hits)/static_cast<double>(regionSize)
  	      << std::endl;
  }
}


int main( int argc, char* argv[] ) {
    // Commandline parsing
  TCLAP::CmdLine cmd("Information about an image.", ' ', VERSION);

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
  //// Commandline parsing is done ////
  
  
  typedef float PixelType;
  const unsigned int Dimension = 3;
  typedef itk::Image< PixelType, Dimension > ImageType;

  // Setup the reader
  typedef itk::ImageFileReader< ImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( imagePath );

  typedef itk::ImageRegionConstIterator< ImageType > ConstIterType;

  std::string commands = "'q' : quit\n's' : Summary statistics\n'r' : ROI overlap estimation\n";
  
  try {
    ImageType::Pointer image  = reader->GetOutput();
    reader->Update();
    ConstIterType constIter( image, reader->GetOutput()->GetRequestedRegion() );

    char command = 'q';
    std::cout << commands << std::endl;
    do {
      std::cin >> command;
      switch (command) {
      case 's':
	{
	  std::set< float > uniqueValues;
	  for ( constIter.GoToBegin(); !constIter.IsAtEnd(); ++constIter ) {
	    uniqueValues.insert( constIter.Get() );
	  }
	  DenseHistogram< float > histogram( uniqueValues.cbegin(), uniqueValues.cend() );
	  
	  for ( constIter.GoToBegin(); !constIter.IsAtEnd(); ++constIter ) {
	  histogram.insert( constIter.Get() );
	  }

	  float old = -1e12;
	  for ( const auto& val : uniqueValues ) {
	    std::cout << '(' << old << ',' << val << "]\t|\t";
	    old = val;
	  }
	  std::cout << '(' << old << ",inf)\t|\t" << std::endl;
	  
	  for ( const auto& count : histogram.getCounts() ) {
	    std::cout << count << "\t|\t";
	  }
	  std::cout << std::endl;
	  break;
	}

      case 'r':
	{
	  EstimateROICoverageFromImage<PixelType,Dimension>( image );
	  break;	  
	}
      case 'q':
	break;
	
      default:
	std::cout << "Unknown command" << std::endl
		  << commands << std::endl;
      }
    } while ( command !=  'q');
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to process." << std::endl
	      << "Image: " << imagePath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Bye" << std::endl;
      
  return EXIT_SUCCESS;
}



void EstimateROICoverage() {
  unsigned int x,y,z;
  unsigned int sizeX,sizeY,sizeZ;

  std::cout << "Volume size (x y z): "; std::cin >> sizeX >> sizeY >> sizeZ;
  std::cout << "ROI size (x y z): "; std::cin >> x >> y >> z;

  unsigned int x0 = x/2, y0 = y/2, z0 = z/2;
  
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> disX(x0, sizeX);
  std::uniform_int_distribution<unsigned int> disY(y0, sizeY);
  std::uniform_int_distribution<unsigned int> disZ(z0, sizeZ);

  typedef itk::Image<bool, 3> ImageType;
  ImageType::Pointer image = ImageType::New();
  image->SetRegions( {{0,0,0}, {sizeX+x0+1,sizeY+y0+1,sizeZ+z0+1}} );
  image->Allocate(true);

  typedef itk::ImageRegionIterator< ImageType > IteratorType;

  std::vector< unsigned int > nSamplesPerRound{ 10, 10, 10, 10, 10, 50, 100, 100, 100, 100, 500 };
  
  // Try different numbers
  unsigned int nROIs = 0;
  for ( auto nSamples : nSamplesPerRound ) {
    nROIs += nSamples;
    for ( unsigned int n = 0; n < nSamples; ++n ) {
      unsigned int cx = disX(gen), cy = disY(gen), cz = disZ(gen);
      unsigned int lx = cx - x0, ly = cy - y0, lz = cz - z0;

      IteratorType iter( image, {{lx,ly,lz}, {x,y,z}} );
      for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
	iter.Set( true );
      }
    }
    IteratorType iter( image, {{x0,y0,z0},{sizeX,sizeY,sizeZ}} );
    unsigned int count = 0;
    for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
      if ( iter.Get() ) {
	++count;
      }
    }
    std::cout << nROIs << " overlap " << static_cast<double>(count)/static_cast<double>(sizeX*sizeY*sizeZ)
	      << std::endl;
  }
}

    
    
  
