#ifndef NormalizedGaussianConvolutionImageFilter_hxx
#define NormalizedGaussianConvolutionImageFilter_hxx

#include "NormalizedGaussianConvolutionImageFilter.h"

namespace itk {

  template< typename TImage >
  NormalizedGaussianConvolutionImageFilter< TImage >
  ::NormalizedGaussianConvolutionImageFilter()
  {
    this->SetNumberOfRequiredInputs(2);
    
    m_MultiplyFilter = MultiplyFilterType::New();
    m_GaussianFilter1 = GaussianFilterType::New();
    m_GaussianFilter2 = GaussianFilterType::New();;
    m_DivideFilter = DivideFilterType::New();
    m_Sigma = 1.0;

    // The filters are connected in GenerateData so it is easier to see what is
    // going 
  }

  template< typename TImage>
  void NormalizedGaussianConvolutionImageFilter<TImage>
  ::SetInputImage(const TImage* image) {
    this->SetNthInput(0, const_cast<TImage*>(image));
  }
 
  template< typename TImage>
  void NormalizedGaussianConvolutionImageFilter<TImage>
  ::SetInputCertainty(const TImage* mask)
  {
    this->SetNthInput(1, const_cast<TImage*>(mask));
  }
  
  template< typename TImage >
  void
  NormalizedGaussianConvolutionImageFilter< TImage >
  ::GenerateData()
  {
    typename ImageType::Pointer inputImage = ImageType::New();
    inputImage->Graft( static_cast<const ImageType * >( this->ProcessObject::GetInput(0) ));

    typename ImageType::Pointer inputCertainty = ImageType::New();
    inputCertainty->Graft( static_cast<const ImageType * >( this->ProcessObject::GetInput(1) ));

    m_MultiplyFilter->SetInput1( inputImage );
    m_MultiplyFilter->SetInput2( inputCertainty );

    m_GaussianFilter1->SetSigma( m_Sigma );
    m_GaussianFilter2->SetSigma( m_Sigma );
    
    m_GaussianFilter1->SetInput( m_MultiplyFilter->GetOutput() );
    m_GaussianFilter2->SetInput( inputCertainty );

    m_DivideFilter->SetInput1( m_GaussianFilter1->GetOutput() );
    m_DivideFilter->SetInput2( m_GaussianFilter2->GetOutput() );   

    m_DivideFilter->GraftOutput( this->GetOutput() );
    m_DivideFilter->Update();
    this->GraftOutput( m_DivideFilter->GetOutput() );
  }

  template< typename TImage >
  void
  NormalizedGaussianConvolutionImageFilter< TImage >
  ::PrintSelf( std::ostream& os, Indent indent ) const
  {
    Superclass::PrintSelf(os,indent);

    os << indent << "Sigma:" << this->m_Sigma
       << std::endl;
  }

} // end namespace itk

#endif
