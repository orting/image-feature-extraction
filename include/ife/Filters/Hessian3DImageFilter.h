/*
  Adapted from itkHessianRecursiveGaussianImageFilter.h
 */
#ifndef __Hessian3DImageFilter_h
#define __Hessian3DImageFilter_h

#include "itkDerivativeImageFilter.h"
#include "itkComposeImageFilter.h"
#include "itkPixelTraits.h"

namespace itk
{
/** \class Hessian3DImageFilter
 * \brief Computes the Hessian matrix of an image by applying a 
 *        second order differential operators.
 *
 *
 * This filter is implemented using the DerivativeImageFilter
 */
// NOTE that the typename macro has to be used here in lieu
// of "typename" because VC++ doesn't like the typename keyword
// on the defaults of template parameters
  template< typename TInputImage,
	    typename TOutputImage=VectorImage<typename TInputImage::PixelType, TInputImage::ImageDimension>
	    >
class Hessian3DImageFilter:
  public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
  /** Standard class typedefs. */
  typedef Hessian3DImageFilter                            Self;
  typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;
  typedef SmartPointer< Self >                            Pointer;
  typedef SmartPointer< const Self >                      ConstPointer;

  /** Pixel Type of the input image */
  typedef TInputImage                                   InputImageType;
  typedef typename TInputImage::PixelType               PixelType;

  /** Pixel Type of the output image */
  typedef TOutputImage                                       OutputImageType;
  typedef typename OutputImageType::PixelType                OutputPixelType;
  
  /** Run-time type information (and related methods).   */
  itkTypeMacro(Hessian3DImageFilter, ImageToImageFilter);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  
  virtual void GenerateOutputInformation(void) ITK_OVERRIDE;

protected:
  typedef itk::DerivativeImageFilter< InputImageType, InputImageType >
  DerivativeFilterType;

  typedef itk::ComposeImageFilter< InputImageType, OutputImageType >
  ComposeFilterType;

  Hessian3DImageFilter();
  virtual ~Hessian3DImageFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

  /** Generate Data */
  void GenerateData(void) ITK_OVERRIDE;

private:
  Hessian3DImageFilter(const Self &); // purposely not implemented
  void operator=(const Self &);       // purposely not implemented

  typename DerivativeFilterType::Pointer m_DxxFilter;
  typename DerivativeFilterType::Pointer m_DxyFilter;
  typename DerivativeFilterType::Pointer m_DxzFilter;
  typename DerivativeFilterType::Pointer m_DyyFilter;
  typename DerivativeFilterType::Pointer m_DyzFilter;
  typename DerivativeFilterType::Pointer m_DzzFilter;

  typename DerivativeFilterType::Pointer m_DxFilter;
  typename DerivativeFilterType::Pointer m_DyFilter;

  typename ComposeFilterType::Pointer m_ComposeFilter;

  };
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "Hessian3DImageFilter.hxx"
#endif

#endif
