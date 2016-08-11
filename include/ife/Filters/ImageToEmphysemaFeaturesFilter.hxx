#ifndef __ImageToEmphysemaFeaturesFilter_hxx
#define __ImageToEmphysemaFeaturesFilter_hxx

#include "ImageToEmphysemaFeaturesFilter.h"

namespace itk
{
/**
 * Constructor
 */
  template< typename TInputImage,
	    typename TInputMask,
	    typename TOutputImage >
  ImageToEmphysemaFeaturesFilter< TInputImage, TInputMask, TOutputImage >
::ImageToEmphysemaFeaturesFilter()
{
  // Default scale
  m_Sigma = 1.0;
  
  // Set mask input to cast filter in GenerateData
  m_CastFilter = CastFilterType::New();

  // Set image input to smoothing filter in GenerateData
  m_SmoothingFilter = SmoothingFilterType::New();
  m_SmoothingFilter->SetInputCertainty( m_CastFilter->GetOutput() );

  m_GradientMagnitudeFilter = GradientMagnitudeFilterType::New();
  m_GradientMagnitudeFilter->SetInput( m_SmoothingFilter->GetOutput() );
  
  m_HessianFilter = HessianFilterType::New();
  m_HessianFilter->SetInput( m_SmoothingFilter->GetOutput() );

  m_EigenvalueFilter = EigenvalueFilterType::New();
  m_EigenvalueFilter->SetInput( m_HessianFilter->GetOutput() );
  m_EigenvalueFilter->SetFunctor( FunctorType() );

  for ( size_t i = 0; i < 6; ++i ) {
    m_IndexSelectionFilters.emplace_back( IndexSelectionFilterType::New() );
    m_IndexSelectionFilters[i]->SetInput( m_EigenvalueFilter->GetOutput() );
    m_IndexSelectionFilters[i]->SetIndex(i);
  }

  // Set the mask input to mask filter in GenerateData
  m_ComposeFilter = ComposeFilterType::New();
  for ( size_t i = 0; i < 8; ++i ) {
    m_MaskFilters.emplace_back( MaskFilterType::New() );
    m_ComposeFilter->SetInput( i, m_MaskFilters[i]->GetOutput() );
  }

  m_MaskFilters[0]->SetInput( m_SmoothingFilter->GetOutput() );
  m_MaskFilters[1]->SetInput( m_GradientMagnitudeFilter->GetOutput() );
  for (size_t i = 2, j = 0; j < 6; ++i, ++j ) {
    m_MaskFilters[i]->SetInput( m_IndexSelectionFilters[j]->GetOutput() );
  }
}

  
  template< typename TInputImage,
	    typename TInputMask,
	    typename TOutputImage >
  void
  ImageToEmphysemaFeaturesFilter< TInputImage, TInputMask, TOutputImage > 
  ::SetInputImage(const TInputImage* image) {
    this->SetNthInput(0, const_cast<TInputImage*>(image));
  }
 
  template< typename TInputImage,
	    typename TInputMask,
	    typename TOutputImage >
  void
  ImageToEmphysemaFeaturesFilter<TInputImage, TInputMask, TOutputImage>
  ::SetInputMask(const TInputMask* mask)
  {
    this->SetNthInput(1, const_cast<TInputMask*>(mask));
  }

  
  template< typename TInputImage,
	    typename TInputMask,
	    typename TOutputImage >
  void
  ImageToEmphysemaFeaturesFilter< TInputImage, TInputMask, TOutputImage >
::GenerateOutputInformation(void)
{
  // Apparently we need to set the numberOfComponentsPerPixel explicitly
  this->Superclass::GenerateOutputInformation();

  OutputImageType *output = this->GetOutput();
  output->SetNumberOfComponentsPerPixel( m_ComposeFilter->GetNumberOfIndexedInputs() );
}
  


  template< typename TInputImage,
	    typename TInputMask,
	    typename TOutputImage >
  void
  ImageToEmphysemaFeaturesFilter< TInputImage, TInputMask, TOutputImage >
  ::GenerateData(void)
  {
    itkDebugMacro(<< "ImageToEmphysemaFeaturesFilter generating data ");

    // Need to graft the inputs
    typename InputImageType::Pointer image = InputImageType::New();
    image->Graft( static_cast<const InputImageType * >( this->ProcessObject::GetInput(0) ));

    typename InputMaskType::Pointer mask = InputMaskType::New();
    mask->Graft( static_cast<const InputMaskType * >( this->ProcessObject::GetInput(1) ));

    m_CastFilter->SetInput( mask );
    m_SmoothingFilter->SetInputImage( image );
    m_SmoothingFilter->SetSigma( m_Sigma );

    for ( auto& maskFilter : m_MaskFilters ) {
      maskFilter->SetMaskImage( mask );
    }
  
    m_ComposeFilter->GraftOutput( this->GetOutput() );
    m_ComposeFilter->Update();
    this->GraftOutput( m_ComposeFilter->GetOutput() );
  }

  template< typename TInputImage,
	    typename TInputMask,
	    typename TOutputImage >
  void
  ImageToEmphysemaFeaturesFilter< TInputImage, TInputMask, TOutputImage >
  ::PrintSelf(std::ostream & os, Indent indent) const
  {
    Superclass::PrintSelf(os, indent);
    os << indent << "Sigma:" << this->m_Sigma
       << std::endl;
  }

} // end namespace itk

#endif
