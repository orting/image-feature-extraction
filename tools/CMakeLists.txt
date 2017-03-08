find_package(Eigen3 REQUIRED)
include_directories( SYSTEM ${EIGEN3_INCLUDE_DIR} )

find_package(ZLIB REQUIRED)
include_directories( SYSTEM ${ZLIB_INCLUDE_DIRS} )

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
  ExtractFeatures
  ExtractLabels
  ExtractMaskedRegion
  ImageBrowser
  MakeBag
  MakeBagOnlyIntensity
  MakeBaggedDataset
  GenerateROIs
  SampleROIs
  ExtractSlices
  ExtractWindow
  ResampleTest
  #UnaryFunctorTest
  )
foreach( prog ${progs} )
  add_executable( ${prog} ${prog}.cxx )
  target_link_libraries( ${prog} ${LIBS} )
  install( TARGETS ${prog} DESTINATION tools )
endforeach( prog )


# find_package(ZLIB REQUIRED)
# include_directories(${ZLIB_INCLUDE_DIRS})

# set( LIBS
#   ${ITK_LIBRARIES}
#   ${ZLIB_LIBRARIES}
#   String
#   HR2Reader
#   )

# set( progs
#   ConvertFromOctave
#   ConvertHR2
#   )
# foreach( prog ${progs} )
#   add_executable( ${prog} ${prog}.cxx )
#   target_link_libraries( ${prog} ${LIBS} )
#   install( TARGETS ${prog} DESTINATION bin )
# endforeach( prog )
