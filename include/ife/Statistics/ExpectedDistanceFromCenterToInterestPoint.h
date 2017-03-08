#ifndef __ExpectedDistanceFromCenterToInterestPoint_h
#define __ExpectedDistanceFromCenterToInterestPoint_h


#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkImageRegionConstIteratorWithIndex.h"

template< typename TMask, typename TProbabilityImage>
double
expectedDistanceFromCenterToInterestPoint( typename TMask::Pointer objectMask,
					   typename TProbabilityImage::Pointer probImage ) {
  typedef TMask MaskType;
  typedef TProbabilityImage ImageType;
  
  typedef itk::SignedMaurerDistanceMapImageFilter< MaskType, ImageType  > DistanceFilterType;
  typename DistanceFilterType::Pointer distanceFilter = DistanceFilterType::New();
  distanceFilter->SetInput( objectMask );
  distanceFilter->SetInsideIsPositive( true );

  typedef itk::MultiplyImageFilter< ImageType > MultiplyFilterType;
  typename MultiplyFilterType::Pointer multiplyFilter = MultiplyFilterType::New();
  multiplyFilter->SetInput1( distanceFilter->GetOutput() );
  multiplyFilter->SetInput2( probImage );
  multiplyFilter->Update(); // Throws
  auto image = multiplyFilter->GetOutput();

  
  typedef itk::ImageRegionConstIteratorWithIndex< MaskType > IteratorType;
  IteratorType iter( objectMask, objectMask->GetRequestedRegion() );

  double sum = 0;
  double n = 0;
  for ( iter.GoToBegin(); !iter.IsAtEnd(); ++iter ) {
    if ( iter.Get() ) {
      ++n;
      sum += image->GetPixel( iter.GetIndex() );
    }
  }

  return n > 0 ? sum/n : 0;
  
}

#endif
