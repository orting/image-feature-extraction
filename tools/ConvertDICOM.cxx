#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "tclap/CmdLine.h"

#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileWriter.h"

#include "ife/Util/Path.h"
#include "ife/Util/String.h"

const std::string VERSION("0.1");

namespace filesys = boost::filesystem;

int main(int argc, char *argv[]) {
  std::string usage = \
    "Convert a DICOM series into a single volume. Beware of cases where slices are not uniformly spaced. "\
    "Output file name is vol<Patient Id>-<Study Date>-<Convolution Kernel>-<Slice Spacing>.<Image Format>";
  
  // Commandline parsing
  TCLAP::CmdLine cmd(usage, ' ', VERSION);

  TCLAP::ValueArg<std::string>
    inArg("i", "input-dir", "Path to input directory.", true, "", "path", cmd);

  TCLAP::ValueArg<std::string>
    outArg("o", "output-directory", "Path to output directory", true, "", "path", cmd);

  TCLAP::ValueArg<std::string>
    imageFormatArg("f", "out-image-format", "File extension indicating output image format", false, "nii.gz", "file extension", cmd);

  TCLAP::ValueArg<bool>
    silentArg("s", "silent", "Suppress non-error output", false, false, "bool", cmd);
  
  try {
    cmd.parse(argc, argv);
  } catch(TCLAP::ArgException &e) {
    std::cerr << "Error : " << e.error() 
	      << " for arg " << e.argId() 
	      << std::endl;
    return EXIT_FAILURE;
  }

  // Store the arguments
  const filesys::path inDir( inArg.getValue() );
  const filesys::path outDir( outArg.getValue() );
  const std::string imageFormat( imageFormatArg.getValue() );
  const bool silent( silentArg.getValue() );
  //// Commandline parsing is done ////

  std::cout << "***********************************  WARNING  **********************************" << std::endl
	    << "* Compiling a DICOM series into a single volume can cause loss of information. *" << std::endl
	    << "* In particular, slice location is assumed to be offset + i * slice_spacing.   *" << std::endl
	    << "***********************************  WARNING  **********************************" << std::endl << std::endl;
   
  constexpr unsigned int Dimension = 3;  
  using PixelType = signed short;
  using ImageType = itk::Image< PixelType, Dimension >;
  using ReaderType = itk::ImageSeriesReader< ImageType >;

  ReaderType::Pointer reader = ReaderType::New();
  using ImageIOType = itk::GDCMImageIO;
  ImageIOType::Pointer dicomIO = ImageIOType::New();
  reader->SetImageIO( dicomIO );
  
  using NamesGeneratorType = itk::GDCMSeriesFileNames;
  NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
  nameGenerator->SetUseSeriesDetails( true );
  nameGenerator->AddSeriesRestriction("0008|0021" );
  nameGenerator->SetDirectory( inDir.string() );

  try {
    nameGenerator->Update();    
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to read DICOM series names." << std::endl
	      << "Indir: " << inDir << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  using WriterType = itk::ImageFileWriter< ImageType >;
  WriterType::Pointer writer =  WriterType::New();
    
  using SeriesIdContainer = std::vector< std::string >;
  const SeriesIdContainer &seriesUID = nameGenerator->GetSeriesUIDs();

  for ( auto it = seriesUID.begin(); it != seriesUID.end();  ++it ) {
    std::string seriesIdentifier = it->c_str();

    if ( !silent ) {
      std::cout << "Reading series: " << seriesIdentifier << std::endl;
    }
    
    using FileNamesContainer = std::vector< std::string >;
    FileNamesContainer fileNames( nameGenerator->GetFileNames( seriesIdentifier ) );  
    reader->SetFileNames( fileNames );    
    writer->SetInput( reader->GetOutput() );
    
    try {	
      reader->Update();
 	
      // Build output name as <Patient Id>-<Study Date>-<Convolution Kernel>-<Slice Spacing>.<Image Format>
      using DictionaryType = itk::MetaDataDictionary;
      using MetaDataStringType = itk::MetaDataObject< std::string >;
      const  DictionaryType & dictionary = dicomIO->GetMetaDataDictionary();

      // Patient Id
      auto it = dictionary.Find("0010|0020");
      std::string patient_id = it != dictionary.End() ? dynamic_cast<MetaDataStringType *>(it->second.GetPointer())->GetMetaDataObjectValue() : "?";

      // Study Date
      it = dictionary.Find("0008|0020");
      std::string study_date = it != dictionary.End() ? dynamic_cast<MetaDataStringType *>(it->second.GetPointer())->GetMetaDataObjectValue() : "?";
      
      // Reconstruction kernel
      it = dictionary.Find("0018|1210");
      std::string recon_kernel = it != dictionary.End() ? dynamic_cast<MetaDataStringType *>(it->second.GetPointer())->GetMetaDataObjectValue() : "?";
      recon_kernel = trim(recon_kernel);

      // Slice spacing
      std::string slice_spacing = std::to_string( reader->GetOutput()->GetSpacing()[2] );
      std::replace(slice_spacing.begin(), slice_spacing.end(), '.', '_');
      slice_spacing = trim(slice_spacing, "_0");

      auto outPath = outDir;
      outPath /= "vol" + patient_id + "-" + study_date + "-" + recon_kernel + "-" + slice_spacing;
      outPath.replace_extension( imageFormat );

      if ( !silent ) {
	std::cout << "Writing to " << outPath.string() << std::endl;
      }

      writer->SetFileName( outPath.string() );
      
      writer->Update();
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to read or write." << std::endl
		<< "In dir:            " << inDir << std::endl
		<< "Series identifier: " << seriesIdentifier << std::endl
		<< "Out dir:           " << outDir << std::endl
		<< "ExceptionObject:   " << e << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
    
