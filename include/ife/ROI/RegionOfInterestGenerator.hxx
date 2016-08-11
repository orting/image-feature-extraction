#ifndef __RegionOfInterestGenerator_hxx
#define __RegionOfInterestGenerator_hxx

#include "RegionOfInterestGenerator.h"

namespace itk {

  template<typename TMask>
  RegionOfInterestGenerator<TMask>::
  RegionOfInterestGenerator(MaskPointer mask) : m_Mask(mask) {} 

  template<typename TMask>
  void
  RegionOfInterestGenerator<TMask>::
  setMask(MaskPointer mask) {
    m_Mask = mask;
  }

  
  template<typename TMask>
  std::vector< typename TMask::RegionType >
  RegionOfInterestGenerator<TMask>::
  generate( size_t numberOfROIs, typename TMask::SizeType size ) {
    m_Mask->Update(); // Might throw

    // We need the image region so we can test that all ROIs are inside the image
    RegionType imageRegion = m_Mask->GetLargestPossibleRegion();
    
    RandomIteratorType iter( m_Mask, imageRegion );
    iter.SetNumberOfSamples( numberOfROIs );
    iter.ReinitializeSeed(); // Because the RandomIterator is deterministic otherwise

    std::vector< RegionType > rois;
    rois.reserve( numberOfROIs );

    // Run the ROI sampling loop  
    for ( size_t nROI = 0; nROI < numberOfROIs;  ) {
      // We use a double loop here. The reason is that we dont actually
      // know how many samples we get from the iterator. If it is a random
      // iterator it will stop after a number of samples and when er call
      // GoToBegin it will continue with a new random sampling with the same
      // amount of samples.
      for ( iter.GoToBegin(); !iter.IsAtEnd() && nROI < numberOfROIs; ++iter ) {
	if ( iter.Get() != 0 ) {
	  // Check if this location is actually inside the image 
	  IndexType start = iter.GetIndex();
	  start[0] -= size[0]/2;
	  start[1] -= size[1]/2;
	  start[2] -= size[2]/2;
	  RegionType roi( start, size );
	  if ( imageRegion.IsInside( roi ) ) {
	    rois.push_back( roi );
	    ++nROI;
	  }
	}
      }
    }
    return rois;
  }
}
#endif
