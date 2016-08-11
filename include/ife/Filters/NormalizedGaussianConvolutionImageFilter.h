#ifndef NormalizedGaussianConvolutionImageFilter_h
#define NormalizedGaussianConvolutionImageFilter_h

/*

   Perform normalized convolution of a 0'th order filter with an image and 
   optionally mask the result with the certainty, such that all voxels with 0 
   certainty are set to 0.
   See:
   Knutsson, Hans and Westin, Carl-Fredrik
   Normalized and Differential Convolution.
   (Specifically section 3.2 0:th order interpolation)
   
   Let B be the filter, T the image, c the certainty of the image and a the
   applicability of the filter.
   
   Let B=1 and denote convolution by *, then we have
   U_N = {a * cT}_N = {a * c}^-1 {a * cT} = {a * cT}/{a * c}.
   
   If we set a = Gauss_sigma and c = binary mask representing a region of 
   interest (ROI), then we get convolution of a Gaussian at scale sigma with T,
   where the influence of values outside the ROI has been reduced   

   Note To Self:
   Should normalization of smoothing be with 1/s² or 1/s? See ITK Software Guide
   version 4.7 page 101 for a discussion.

   // For the derivative version
   T is the image
   c is the certainty mask
   a is the Gaussian applicability function
   B is the constant filter

   See Knutsson section 3.2 0'th order interpolation.
   We get
   {a * c}^-1 {a * cT} = {a * cT}/{a * c}

   If we differentiate that expression we get

   d/dx [{a * cT}/{a * c}] = 
   ([d/dx {a * cT}]{a * c} - [d/dx {a * c}]{a * cT}) / ({a * c}^2) =
   ({[d/dx a] * cT}{a * c} - {[d/dx a] * c}{a * cT}) / ({a * c}^2)

   So we can just differentiate the Gaussian a.

 */
#include "itkDivideImageFilter.h"
#include "itkImageToImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"

namespace itk {
  
  template < typename TImage >
  class NormalizedGaussianConvolutionImageFilter :
    public ImageToImageFilter< TImage, TImage >  {
    
  public:
    typedef NormalizedGaussianConvolutionImageFilter Self;
    typedef ImageToImageFilter< TImage, TImage >     Superclass;
    typedef SmartPointer< Self >                     Pointer;
    typedef SmartPointer< const Self >               ConstPointer;

    typedef TImage                        ImageType;

    // We wan to hide the composited filters, so they are protected. But we need
    // the ScalarRealType from the Gaussian smoothing filter, so we need to
    // define the typedefs here.
  protected:
    typedef MultiplyImageFilter< ImageType > MultiplyFilterType;
    typedef DivideImageFilter< ImageType, ImageType, ImageType > DivideFilterType;
    typedef SmoothingRecursiveGaussianImageFilter< ImageType, ImageType > GaussianFilterType;

  public:
    typedef typename GaussianFilterType::ScalarRealType ScalarRealType;

    /** Method for creation through object factory */
    itkNewMacro(Self);

    /** Run-time type information */
    itkTypeMacro(NormalizedGaussianConvolutionimageFilter,
		 ImageToImageFilter);


    /** The image to convolve */
    void SetInputImage(const ImageType* image);

    /** The certainty of pixels in the input image */
    void SetInputCertainty(const ImageType* image);

    /** Get/Set the scale of the Gaussian */
    itkGetMacro( Sigma, ScalarRealType );
    itkSetMacro( Sigma, ScalarRealType );

  protected:
    NormalizedGaussianConvolutionImageFilter();
  
    virtual void GenerateData() ITK_OVERRIDE;

    /** Display */
    void PrintSelf( std::ostream& os, Indent indent ) const ITK_OVERRIDE;
    
  private:

    NormalizedGaussianConvolutionImageFilter(Self&);   // purposely not implemented
    void operator=(const Self&);          // purposely not implemented

    typename MultiplyFilterType::Pointer m_MultiplyFilter;
    typename GaussianFilterType::Pointer m_GaussianFilter1;
    typename GaussianFilterType::Pointer m_GaussianFilter2;
    typename DivideFilterType::Pointer   m_DivideFilter;
    ScalarRealType m_Sigma;
  };

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "NormalizedGaussianConvolutionImageFilter.hxx"
#endif

#endif
