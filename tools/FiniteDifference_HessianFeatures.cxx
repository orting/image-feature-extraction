/* 
   Calculate features derived from the Hessian estimated in each voxel, and 
   masked with mask image.
   The Hessian is estimated by convolution with 2'nd order derivative operators.
   These appear to be based on central differences, see 
   http://www.itk.org/Doxygen48/html/classitk_1_1DerivativeOperator.html
   Uses ZeroFLuxNeumannBoundaryCondition.

   The calculated features are 
   * Eigenvalues ordered by magnitude such that |eig1| >= |eig2| >= |eig3|.

   * Laplacian of Gauss (eig1 + eig2 + eig3)

   * Gaussian curvature (eig1 * eig2 * eig3)
   
   * Frobenius norm (sqrt(eig1^2 + eig2^2 + eig3^2))


*/
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkDerivativeImageFilter.h"
#include "itkComposeImageFilter.h"
#include "itkFixedArray.h"
#include "itkVectorImageToImageAdaptor.h"
#include "itkVectorIndexSelectionCastImageFilter.h"

#include "ife/Util/Path.h"
#include "Eigenvalues.h"

const std::string VERSION("0.1");
const std::string OUT_FILE_TYPE(".nii.gz");

int main(int argc, char *argv[]) {
  // Commandline parsing
  TCLAP::CmdLine cmd("Calculate Hessian based features.", ' ', VERSION);

  // We need a single image
  TCLAP::ValueArg<std::string> 
    imageArg("i", 
	     "image", 
	     "Path to image.",
	     true,
	     "",
	     "path", 
	     cmd);

  // We need a single mask
  TCLAP::ValueArg<std::string> 
    maskArg("m", 
	    "mask", 
	    "Path to mask. Must match image dimensions.",
	    true,
	    "",
	    "path", 
	    cmd);
  
  // We need a directory for storing the resulting images
  TCLAP::ValueArg<std::string> 
    outDirArg("o", 
	      "outdir", 
	      "Path to output directory",
	      true, 
	      "", 
	      "path", 
	      cmd);
  
  // We can use a prefix to generate filenames
  TCLAP::ValueArg<std::string> 
    prefixArg("p", 
	      "prefix", 
	      "Prefix to use for output filenames",
	      false, 
	      "hessian_", 
	      "string", 
	      cmd);

  
  try {
    cmd.parse(argc, argv);
  } catch(TCLAP::ArgException &e) {
    std::cerr << "Error : " << e.error() 
	      << " for arg " << e.argId() 
	      << std::endl;
    return EXIT_FAILURE;
  }

  // Store the arguments
  std::string imagePath( imageArg.getValue() );
  std::string maskPath( maskArg.getValue() );
  std::string outDirPath( outDirArg.getValue() );
  std::string prefix( prefixArg.getValue() );

  //// Commandline parsing is done ////


  // Some common values/types that are always used.
  const unsigned int Dimension = 3;

  // TODO: Need to consider which pixeltype to use
  typedef float PixelType;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::VectorImage< PixelType, Dimension >  VectorImageType;
  typedef itk::ImageFileReader< ImageType > ReaderType;
  
  typedef unsigned char MaskPixelType;
  typedef itk::Image< MaskPixelType, Dimension >  MaskType;
  typedef itk::ImageFileReader< MaskType > MaskReaderType;

  
    
  // Setup the readers
  ReaderType::Pointer imageReader = ReaderType::New();
  imageReader->SetFileName( imagePath );

  MaskReaderType::Pointer maskReader = MaskReaderType::New();
  maskReader->SetFileName( maskPath );

  
  // Setup the derivative filters. We need six to get all needed derivatives
  typedef itk::DerivativeImageFilter< ImageType, ImageType >
    DerivativeFilterType;

  // The second order derivatives
  DerivativeFilterType::Pointer dxxFilter = DerivativeFilterType::New();
  dxxFilter->SetOrder( 2 );
  dxxFilter->SetDirection( 0 );
  dxxFilter->SetInput( imageReader->GetOutput() );

  DerivativeFilterType::Pointer dyyFilter = DerivativeFilterType::New();
  dyyFilter->SetOrder( 2 );
  dyyFilter->SetDirection( 1 );
  dyyFilter->SetInput( imageReader->GetOutput() );

  DerivativeFilterType::Pointer dzzFilter = DerivativeFilterType::New();
  dzzFilter->SetOrder( 2 );
  dzzFilter->SetDirection( 2 );
  dzzFilter->SetInput( imageReader->GetOutput() );

  // The cross derivatives
  // First we need first order derivatives in x and y direction
  DerivativeFilterType::Pointer dxFilter = DerivativeFilterType::New();
  dxFilter->SetOrder( 1 );
  dxFilter->SetDirection( 0 );
  dxFilter->SetInput( imageReader->GetOutput() );

  DerivativeFilterType::Pointer dyFilter = DerivativeFilterType::New();
  dyFilter->SetOrder( 1 );
  dyFilter->SetDirection( 0 );
  dyFilter->SetInput( imageReader->GetOutput() );

  // Now we can do the cross derivatives
  DerivativeFilterType::Pointer dxyFilter = DerivativeFilterType::New();
  dxyFilter->SetOrder( 1 );
  dxyFilter->SetDirection( 1 );
  dxyFilter->SetInput( dxFilter->GetOutput() );

  DerivativeFilterType::Pointer dxzFilter = DerivativeFilterType::New();
  dxzFilter->SetOrder( 1 );
  dxzFilter->SetDirection( 2 );
  dxzFilter->SetInput( dxFilter->GetOutput() );

  DerivativeFilterType::Pointer dyzFilter = DerivativeFilterType::New();
  dyzFilter->SetOrder( 1 );
  dyzFilter->SetDirection( 2 );
  dyzFilter->SetInput( dyFilter->GetOutput() );

  // Now we combine the derivatives into a vector image
  typedef itk::ComposeImageFilter< ImageType > ImageToVectorFilterType;
  ImageToVectorFilterType::Pointer imageToVectorFilter =
    ImageToVectorFilterType::New();
  imageToVectorFilter->SetInput( 0, dxxFilter->GetOutput() );
  imageToVectorFilter->SetInput( 1, dxyFilter->GetOutput() );
  imageToVectorFilter->SetInput( 2, dxzFilter->GetOutput() );
  imageToVectorFilter->SetInput( 3, dyyFilter->GetOutput() );
  imageToVectorFilter->SetInput( 4, dyzFilter->GetOutput() );
  imageToVectorFilter->SetInput( 5, dzzFilter->GetOutput() );

  VectorImageType::Pointer hessianImage = imageToVectorFilter->GetOutput();

  // We need an explicit update before we start iterating over the images and
  // mask 
  try {
    hessianImage->Update();
    maskReader->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "An error occurred: " << e << std::endl;
    return EXIT_FAILURE;
  }
  
  // Now we iterate over each voxel and calculate the eigen values.
  // This could be moved into a separate filter, but that is TODO.
  // We just store the eigenvalues in the three first components of hessianImage
  // the LoG, Gausian curvature and Frobenius norm in the last three.
  typedef itk::ImageRegionConstIterator< MaskType> MaskIteratorType;
  MaskIteratorType maskIterator( maskReader->GetOutput(), 
  				 maskReader->GetOutput()->GetRequestedRegion() );

  typedef itk::ImageRegionIterator< VectorImageType> VectorIteratorType;
  VectorIteratorType hessianIterator( hessianImage,
				      hessianImage->GetRequestedRegion() );
  for ( hessianIterator.GoToBegin(), maskIterator.GoToBegin();
	!(hessianIterator.IsAtEnd() || maskIterator.IsAtEnd());
	++hessianIterator, ++maskIterator ) {
    // Need a mask iterator here to skip pixels outside the mask
    if ( maskIterator.Get() == 0 ) {
      itk::VariableLengthVector<PixelType> pixel( 6 );
      pixel.Fill( 0 ); // Not sure this is necesary, check source of vector type
      hessianIterator.Set( pixel );
    }
    else {
      auto pixel = hessianIterator.Get();
      itk::FixedArray<PixelType,6> hessian{ pixel.GetDataPointer() };
      itk::FixedArray<PixelType,3> eig = eigenvalues_symmetric3x3( hessian );
      pixel[0] = eig[0];
      pixel[1] = eig[1];
      pixel[2] = eig[2];
      pixel[3] = eig[0] + eig[1] + eig[2];
      pixel[4] = eig[0] * eig[1] * eig[2];
      pixel[5] = std::sqrt(eig[0]*eig[0] + eig[1]*eig[1] + eig[2]*eig[2]);
    }
  }
  
  /* 
   * This does not work, because ImageFileWriter does not work with ImageAdaptor
   * See: http://www.itk.org/pipermail/insight-users/2007-November/024188.html
   * If a solution is found/exists it should be used, because adaptors do not
   * allocate new space for the image, which a filter does.
   */
  // Setup an adaptor that extracts the features one at a tiime
  // typedef itk::VectorImageToImageAdaptor< PixelType, Dimension > AdaptorType;
  // AdaptorType::Pointer adaptor = AdaptorType::New();
  // adaptor->SetImage( hessianImage );
  /* Instead we use a filter */
  typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ImageType>
    IndexSelectionType;
  IndexSelectionType::Pointer indexSelectionFilter = IndexSelectionType::New();
  indexSelectionFilter->SetInput( hessianImage );
  
  // Setup the writer
  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetInput( indexSelectionFilter->GetOutput() );

  // Base file name for output images
  std::string baseFileName = Path::join( outDirPath, prefix );

  std::vector< std::string > featureNames{
    "eig1", "eig2", "eig3","LoG", "Curvature", "Frobenius" 
      };
  
  for (unsigned int i = 0;
       i < hessianImage->GetNumberOfComponentsPerPixel();
       ++i ) {
    indexSelectionFilter->SetIndex( i );
    // Create a filename
    std::string outFile = baseFileName + featureNames.at( i ) + OUT_FILE_TYPE;
    writer->SetFileName( outFile );
    try {
      writer->Update();
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to process." << std::endl
  		<< "Image: " << imagePath << std::endl
  		<< "Mask: " << maskPath << std::endl
  		<< "Base file name: " << baseFileName << std::endl
  		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
