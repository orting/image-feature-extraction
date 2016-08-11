#ifndef __Hessian3DImageFilter_hxx
#define __Hessian3DImageFilter_hxx

#include "Hessian3DImageFilter.h"

namespace itk
{
/**
 * Constructor
 */
template< typename TInputImage, typename TOutputImage >
Hessian3DImageFilter< TInputImage, TOutputImage >
::Hessian3DImageFilter()
{

  // I think the way to do this is with a map, where the keys are
  // (direction, order)
  
  m_DxxFilter = DerivativeFilterType::New();
  m_DyyFilter = DerivativeFilterType::New();
  m_DzzFilter = DerivativeFilterType::New();
  
  m_DxxFilter->SetOrder(2);
  m_DxxFilter->SetDirection(0);
  m_DyyFilter->SetOrder(2);
  m_DyyFilter->SetDirection(1);
  m_DzzFilter->SetOrder(2);
  m_DzzFilter->SetDirection(2);

  
  m_DxFilter = DerivativeFilterType::New();
  m_DyFilter = DerivativeFilterType::New();

  m_DxFilter->SetOrder(1);
  m_DxFilter->SetDirection(0);
  m_DyFilter->SetOrder(1);
  m_DyFilter->SetDirection(1);
  
  m_DxyFilter = DerivativeFilterType::New();
  m_DxzFilter = DerivativeFilterType::New();
  m_DyzFilter = DerivativeFilterType::New();

  m_DxyFilter->SetOrder(1);
  m_DxyFilter->SetDirection(1);
  m_DxyFilter->SetInput( m_DxFilter->GetOutput() );
  m_DxzFilter->SetOrder(1);
  m_DxzFilter->SetDirection(2);
  m_DxzFilter->SetInput( m_DxFilter->GetOutput() );
  m_DyzFilter->SetOrder(1);
  m_DyzFilter->SetDirection(2);
  m_DyzFilter->SetInput( m_DyFilter->GetOutput() );

  m_ComposeFilter = ComposeFilterType::New();
  m_ComposeFilter->SetInput( 0, m_DxxFilter->GetOutput() );
  m_ComposeFilter->SetInput( 1, m_DxyFilter->GetOutput() );
  m_ComposeFilter->SetInput( 2, m_DxzFilter->GetOutput() );
  m_ComposeFilter->SetInput( 3, m_DyyFilter->GetOutput() );
  m_ComposeFilter->SetInput( 4, m_DyzFilter->GetOutput() );
  m_ComposeFilter->SetInput( 5, m_DzzFilter->GetOutput() ); 
}

  
template< typename TInputImage, typename TOutputImage >
void
Hessian3DImageFilter< TInputImage, TOutputImage >
::GenerateOutputInformation(void)
{
  // Apparently we need to set the numberOfComponentsPerPixel explicitly
  this->Superclass::GenerateOutputInformation();

  OutputImageType *output = this->GetOutput();
  output->SetNumberOfComponentsPerPixel( m_ComposeFilter->GetNumberOfIndexedInputs() );
}
  


template< typename TInputImage, typename TOutputImage >
void
Hessian3DImageFilter< TInputImage, TOutputImage >
::GenerateData(void)
{
  itkDebugMacro(<< "Hessian3DImageFilter generating data ");

  // Need to graft it
  typename InputImageType::Pointer inputImage = InputImageType::New();
  inputImage->Graft( const_cast< InputImageType * >( this->GetInput() ));
  
  m_DxxFilter->SetInput( inputImage );
  m_DyyFilter->SetInput( inputImage );
  m_DzzFilter->SetInput( inputImage );
  m_DxFilter->SetInput( inputImage );
  m_DyFilter->SetInput( inputImage );

  m_ComposeFilter->GraftOutput( this->GetOutput() );
  m_ComposeFilter->Update();
  this->GraftOutput( m_ComposeFilter->GetOutput() );
}

template< typename TInputImage, typename TOutputImage >
void
Hessian3DImageFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif
