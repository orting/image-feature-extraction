#ifndef __DenseROIGenerator_hxx
#define __DenseROIGenerator_hxx

#include "DenseROIGenerator.h"

namespace itk {

  template<typename TMask>
  DenseROIGenerator<TMask>::
  DenseROIGenerator(MaskPointer mask) : m_Mask(mask) {} 

  template<typename TMask>
  void
  DenseROIGenerator<TMask>::
  setMask(MaskPointer mask) {
    m_Mask = mask;
  }

  
  template<typename TMask>
  std::vector< typename TMask::RegionType >
  DenseROIGenerator<TMask>::
  generate( typename TMask::SizeType size ) {
    m_Mask->Update(); // Might throw

    // We need the image region so we can test that all ROIs are inside the image
    RegionType imageRegion = m_Mask->GetLargestPossibleRegion();
    
    IteratorType iter( m_Mask, imageRegion );
    std::vector< RegionType > rois;

    for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
      if ( iter.Get() != 0 ) {
	// Check if this location is actually inside the image 
	IndexType start = iter.GetIndex();
	start[0] -= size[0]/2;
	start[1] -= size[1]/2;
	start[2] -= size[2]/2;
	RegionType roi( start, size );
	if ( imageRegion.IsInside( roi ) ) {
	  rois.push_back( roi );
	}
      }
    }
    return rois;
  }
}
#endif
