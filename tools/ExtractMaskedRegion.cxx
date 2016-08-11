/* 
   Extract part of a mask
 */
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkUnaryFunctorImageFilter.h"

#include "ife/Util/Path.h"

const std::string VERSION("0.1");
const std::string OUT_FILE_TYPE(".nii.gz");

// A Functor that returns inside if a pixel value is equal to one from a predefined set
// and return outside otherwise.
template< typename PixelType >
struct MembershipFunctor {
  typedef MembershipFunctor< PixelType > Self;
  
  MembershipFunctor()
    : m_Set( ),
      m_Inside( 0 ),
      m_Outside( 1 )
  {}
    
  MembershipFunctor( const std::vector< PixelType >& set,
		     PixelType inside,
		     PixelType outside )
    : m_Set( set ),
      m_Inside( inside ),
      m_Outside( outside )
  {}

  Self& operator=(Self&& other) {
    m_Set = std::move( other.m_Set );
    m_Inside = std::move( other.m_Inside );
    m_Outside = std::move( other.m_Outside );
    return *this;
  }

  Self& operator=(const Self& other) {
    m_Set = other.m_Set;
    m_Inside = other.m_Inside;
    m_Outside = other.m_Outside;
    return *this;
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
  inline bool operator!=( const MembershipFunctor& other ) const {
    return false;
  }
#pragma GCC diagnostic pop
  
  inline bool operator==( const MembershipFunctor& other ) const {
    return !( *this != other );
  }
  
  inline PixelType operator()( const PixelType& p ) const {
    if ( std::binary_search( m_Set.cbegin(), m_Set.cend(), p ) ) {
      return m_Inside;
    }
    return m_Outside;
  }

private:
  std::vector< PixelType > m_Set;
  PixelType m_Inside, m_Outside;
};



int main(int argc, char *argv[]) {
  typedef unsigned short PixelType;
  
  // Commandline parsing
  TCLAP::CmdLine cmd("Extract part of a mask.", ' ', VERSION);

  TCLAP::ValueArg<std::string> 
    maskArg("m", 
	    "mask", 
	    "Path to mask.",
	    true,
	    "",
	    "path", 
	    cmd);
  
  TCLAP::ValueArg<std::string> 
    outArg("o", 
	   "out", 
	   "Path to output file",
	   true, 
	   "", 
	   "path", 
	   cmd);

  TCLAP::MultiArg<PixelType> 
    includeArg("i", 
	       "include", 
	       "Mask values to include",
	       true, 
	       "PixelType", 
	       cmd);

  TCLAP::ValueArg<PixelType> 
    insideArg("I", 
	      "inside", 
	      "Value to use for pixels that are included",
	       false, 
	       1,
	       "PixelType", 
	       cmd);

  TCLAP::ValueArg<PixelType> 
    outsideArg("O", 
	       "outside", 
	       "Value to use for pixels that are not included",
	       false, 
	       0,
	       "PixelType", 
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
  std::string maskPath( maskArg.getValue() );
  std::string outPath( outArg.getValue() );
  std::vector< PixelType > include ( includeArg.getValue() );
  PixelType inside ( insideArg.getValue() );
  PixelType outside ( outsideArg.getValue() );
  //// Commandline parsing is done ////

  // We are gonna do a lot of queries on the include values, so we sort them so
  // we can use binary search.
  // We could also use an unordered_map. TODO: Check which is fastest
  std::sort( include.begin(), include.end() );


  // Some common values/types that are always used.
  const unsigned int Dimension = 3;
  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef itk::ImageFileReader< ImageType > ReaderType;
    
  // Setup the reader
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( maskPath );


  // Setup the filter
  typedef MembershipFunctor< PixelType > FunctorType;
  typedef itk::UnaryFunctorImageFilter< ImageType, ImageType, FunctorType > FilterType;
  
  FilterType::Pointer filter = FilterType::New();
  filter->SetInput( reader->GetOutput() );
  filter->SetFunctor( FunctorType( include, inside, outside ) );

  // Setup the writer
  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetInput( filter->GetOutput() );
  writer->SetFileName( outPath );
  
  try {
    writer->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to process." << std::endl
	      << "Mask: " << maskPath << std::endl
	      << "Out: " << outPath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
