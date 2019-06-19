#ifndef __DenseROIGenerator_h
#define __DenseROIGenerator_h

#include <vector>
#include "itkImageRegionConstIteratorWithIndex.h"

namespace itk {
  template< typename TMask >
  class DenseROIGenerator {
  public:
    typedef TMask MaskType;
    typedef typename MaskType::Pointer MaskPointer;
    typedef typename MaskType::IndexType IndexType;
    typedef typename MaskType::SizeType SizeType;
    typedef typename MaskType::RegionType RegionType;
  
    DenseROIGenerator(MaskPointer mask);

    void setMask(MaskPointer mask);
    
    std::vector< RegionType >
    generate( SizeType size );

  protected:
    typedef ImageRegionConstIteratorWithIndex< MaskType > IteratorType;
  
  private:
    MaskPointer m_Mask;
  };
}
#ifndef ITK_MANUAL_INSTANTIATION
#include "DenseROIGenerator.hxx"
#endif

#endif
