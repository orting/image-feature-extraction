
// We need an image, a mask, a histogram specification and optionally a set of ROIs
#include <cassert>
#include <fstream>
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "ife/IO/IO.h"
#include "bd/BaggedDataset.h"

const std::string VERSION("0.1");

int main(int argc, char *argv[]) {  
  //// Commandline parsing ////
  TCLAP::CmdLine cmd("Create a bagged dataset from a collection of bags.", ' ', VERSION);

  // Required arguments  
  TCLAP::ValueArg<std::string> 
    bagListArg("b", 
	       "bag-list", 
	       "Path to bag list.",
	       true,
	       "",
	       "path", 
	       cmd);

  TCLAP::ValueArg<std::string> 
    bagLabelArg("l", 
		"bag-labels", 
		"Path to bag labels",
		true, 
		"", 
		"string", 
		cmd);

  TCLAP::ValueArg<std::string> 
    outArg("o", 
	   "outpath", 
	   "Path to store dataset at",
	   true, 
	   "", 
	   "string", 
	   cmd);
    
  // Optional arguments
  TCLAP::ValueArg<std::string> 
    instanceLabelArg("L", 
		     "instance-labels", 
		     "Path to instance labels",
		     true, 
		     "", 
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
  const std::string bagListPath( bagListArg.getValue() );
  const std::string bagLabelPath( bagLabelArg.getValue() );
  const std::string outPath( outArg.getValue() );
  const std::string instanceLabelPath( instanceLabelArg.getValue() );
  //// Commandline parsing is done ////

  typedef BaggedDataset< Eigen::Dynamic, Eigen::Dynamic > BaggedDatasetType;

  // Read instances
  std::vector< size_t > indices;
  std::vector< std::string > paths;
  std::ifstream isBagList( bagListPath );
  std::string bagPath;
  std::vector< double > bufInstances;
  char colSep = ',', rowSep = '\n';
  bool firstBag = true;
  size_t cols = 0;
  size_t rows = 0;
  size_t bagIdx = 0;
  while ( isBagList >> bagPath ) {
    std::ifstream isBag ( bagPath );
    auto dataDim = readTextMatrix< double >( isBag, std::back_inserter(bufInstances), colSep, rowSep );
    if ( firstBag ) {
      cols = dataDim.second;
      firstBag = false;
    }
    if ( cols != dataDim.second ) {
      std::cerr << "Number of columns in bags do no not match" << std::endl;
      return EXIT_FAILURE;
    }
    rows += dataDim.first;

    // Add bag index for each instance from this bag
    std::fill_n( std::back_inserter( indices ), dataDim.first, bagIdx );
    ++bagIdx;    
  }
  
  // Check that we didnt mess up the counting
  if ( rows != indices.size() ) {
    std::cerr << "Number of indices does not match number of instances" << std::endl;
    return EXIT_FAILURE;
  }
  
  BaggedDatasetType::MatrixType instances( rows, cols );
  std::copy( bufInstances.cbegin(), bufInstances.cend(), instances.data() );

  BaggedDatasetType::IndexVectorType bagMembershipIndices( rows );
  std::copy( indices.cbegin(), indices.cend(), bagMembershipIndices.data() );

  // Read bag labels
  std::ifstream isBagLabels( bagLabelPath );
  std::vector< double > bufBagLabels;
  auto dimBagLabels = readTextMatrix< double >( isBagLabels, std::back_inserter( bufBagLabels ), colSep, rowSep );
  if ( dimBagLabels.first != bagIdx ) {
    std::cerr << "Number of bag labels does not match number of bags" << std::endl;
    return EXIT_FAILURE;
  }
  BaggedDatasetType::BagLabelVectorType bagLabels( dimBagLabels.first, dimBagLabels.second );
  std::copy( bufBagLabels.cbegin(), bufBagLabels.cend(), bagLabels.data() );

  // Read instance labels if we got them
  BaggedDatasetType::InstanceLabelVectorType instanceLabels;  
  if ( instanceLabelPath.empty() ) {
    instanceLabels = BaggedDatasetType::InstanceLabelVectorType::Zero( rows, 1 );
  }
  else {
    std::ifstream isInstanceLabels( instanceLabelPath );
     std::vector< double > bufInstanceLabels;
     auto dimInstanceLabels = readTextMatrix< double >( isInstanceLabels, std::back_inserter( bufInstanceLabels ), colSep, rowSep );
     if ( dimInstanceLabels.first != rows ) {
       std::cerr << "Number of instance labels does not match number of instances" << std::endl;
       return EXIT_FAILURE;
     }
     instanceLabels.resize( dimInstanceLabels.first, dimInstanceLabels.second );
     std::copy( bufInstanceLabels.cbegin(), bufInstanceLabels.cend(), instanceLabels.data() );
  }

  BaggedDatasetType bags( instances, bagMembershipIndices, bagLabels, instanceLabels );
  std::ofstream out( outPath );
  if ( ! bags.Save( out ) ) {
    std::cerr << "Error writing dataset to " << outPath << std::endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
