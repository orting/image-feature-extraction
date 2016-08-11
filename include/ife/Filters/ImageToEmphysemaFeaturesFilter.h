#ifndef __ImageToEmphysemaFeaturesFilter_h
#define __ImageToEmphysemaFeaturesFilter_h

/*
  Extract features used for estimating emphysema in CT lung scans.
  See: ... TODO ...

 */
#include "itkGradientMagnitudeImageFilter.h"
#include "itkComposeImageFilter.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkMaskImageFilter.h"

#include "ife/Numerics/EigenvalueFeaturesFunctor.h"
#include "ife/Filters/Hessian3DImageFilter.h"
#include "ife/Filters/NormalizedGaussianConvolutionImageFilter.h"

namespace itk {
  
  template < typename TInputImage,
	     typename TInputMask,
	     typename TOutputImage >
  class ImageToEmphysemaFeaturesFilter :
    public ImageToImageFilter< TInputImage, TOutputImage >  {
    
  public:
    typedef ImageToEmphysemaFeaturesFilter                  Self;
    typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;
    typedef SmartPointer< Self >                            Pointer;
    typedef SmartPointer< const Self >                      ConstPointer;

    typedef TInputImage InputImageType;
    typedef TInputMask InputMaskType;
    typedef TOutputImage OutputImageType;

    typedef typename InputImageType::PixelType PixelType;
    typedef typename OutputImageType::PixelType OutputPixelType;

    // We must have scalar valued input
    typedef PixelType ScalarRealType;
    
    /** Method for creation through object factory */
    itkNewMacro(Self);

    /** Run-time type information */
    itkTypeMacro(ImageToEmphysemaFeaturesFilter,
		 ImageToImageFilter);


    /** The image to convolve */
    void SetInputImage(const InputImageType* image);

    /** A lung mask. Assumed to be binary valued. */
    void SetInputMask(const InputMaskType* image);

    /** Get/Set the scale in mm at which to calculate features */
    itkGetMacro( Sigma, ScalarRealType );
    itkSetMacro( Sigma, ScalarRealType );

    /** We calculate 8 different features */
    static const size_t numFeatures = 8;
    
    virtual void GenerateOutputInformation(void) ITK_OVERRIDE;
    
  protected:
    /** Smoothing filter */
    typedef NormalizedGaussianConvolutionImageFilter<
    InputImageType > SmoothingFilterType;

    
    /** We need to ensure that the mask is treated the same way as the
	image when we do normalized convolution */
    typedef CastImageFilter<
      InputMaskType,
      InputImageType > CastFilterType;

   
    /** Filter that calculates gradient magnitude without smoothing */
    typedef GradientMagnitudeImageFilter<
      InputImageType,
      InputImageType > GradientMagnitudeFilterType;

    
    /** Filter that calculates the Hessian without smoothing */
    typedef Hessian3DImageFilter<
      InputImageType,
      OutputImageType > HessianFilterType;

    
    /** Filter that calculates the Hessian based features
	eig1, eig2, eig3, Laplacian of Gaussian, Gaussian curvature, 
	Frobenius norm */
    typedef EigenvalueFeaturesFunctor< PixelType > FunctorType;
    typedef UnaryFunctorImageFilter<
      OutputImageType,
      OutputImageType,
      FunctorType > EigenvalueFilterType;

    
    /** Filter that extracts indivual features from the feature filters */
    typedef VectorIndexSelectionCastImageFilter<
      OutputImageType,
      InputImageType > IndexSelectionFilterType;

    
    /** Filter that masks the feature images with the mask */
    typedef MaskImageFilter<
      InputImageType,
      InputMaskType,
      InputImageType > MaskFilterType;

    
    /** Filter that combines the feature images into a single image */
    typedef ComposeImageFilter<
      InputImageType,
      OutputImageType > ComposeFilterType;

    
    ImageToEmphysemaFeaturesFilter();
    virtual ~ImageToEmphysemaFeaturesFilter(){};
  
    virtual void GenerateData() ITK_OVERRIDE;

    /** Display */
    void PrintSelf( std::ostream& os, Indent indent ) const ITK_OVERRIDE;
    
  private:
    ImageToEmphysemaFeaturesFilter(Self&);   // purposely not implemented
    void operator=(const Self&);          // purposely not implemented

    // The filters
    typename SmoothingFilterType::Pointer m_SmoothingFilter;
    typename CastFilterType::Pointer m_CastFilter;
    
    typename GradientMagnitudeFilterType::Pointer m_GradientMagnitudeFilter;
    typename HessianFilterType::Pointer m_HessianFilter;
    typename EigenvalueFilterType::Pointer m_EigenvalueFilter;
    std::vector< typename IndexSelectionFilterType::Pointer > m_IndexSelectionFilters;
    std::vector< typename MaskFilterType::Pointer > m_MaskFilters;
    typename ComposeFilterType::Pointer m_ComposeFilter;

    // The parameters
    ScalarRealType m_Sigma;
  };

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "ImageToEmphysemaFeaturesFilter.hxx"
#endif

#endif
