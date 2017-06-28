find_package(Eigen3 REQUIRED)
include_directories( SYSTEM ${EIGEN3_INCLUDE_DIR} )

find_package(ZLIB REQUIRED)
include_directories( SYSTEM ${ZLIB_INCLUDE_DIRS} )

include_directories( "/home/pcn178/local/include/bd" )
include_directories( "/home/silas/Projects/bagged-data/include/" )

set( LIBS
  ${ITK_LIBRARIES}
  ${ZLIB_LIBRARIES}
  IO
  String
  HR2Reader
  )

set( progs
  ConvertFromOctave
  ConvertHR2
  DetermineHistogramBinEdges_MultiScaleEigenvalueFeatures
  MaskedImageFilter
  MaskedNormalizedConvolution
  #FiniteDifference_HessianFeatures
  FiniteDifference_GradientFeatures
  CalculateExpectedDistanceFromCenterToInterestPoints
  ExtractBoundingBox
  ExtractFeatures
  ExtractLabels
  ExtractMaskedRegion
  ImageBrowser
  MakeBag
  MakeBagOnlyIntensity
  MakeBaggedDataset
  GenerateROIs
  PadImage
  SampleROIs
  ExtractSlices
  ExtractWindow
  )

foreach( prog ${progs} )
  add_executable( ${prog} ${prog}.cxx )
  target_link_libraries( ${prog} ${LIBS} )
  install( TARGETS ${prog} DESTINATION tools )
endforeach( prog )
