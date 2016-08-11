cmake_minimum_required( VERSION 2.8.6 FATAL_ERROR )

# We want C++11
set( GCC_FLAGS "-Wall -Wextra -Wpedantic" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS} -std=c++11" CACHE STRING "Flags used by the compiler during all build types")

project( image-feature-extraction )

option( BUILD_TOOLS "Build tools" ON )
option( BUILD_TESTING "Build tests" ON )

find_package( ITK REQUIRED )
include( ${ITK_USE_FILE} )
include_directories( SYSTEM ${ITK_INCLUDE_DIRS} )

#find_package(Eigen3 REQUIRED)
#include_directories( SYSTEM ${EIGEN3_INCLUDE_DIR} )

include_directories( include )

add_subdirectory( src )


if( BUILD_TOOLS )
  add_subdirectory( tools )
endif( BUILD_TOOLS )

if( BUILD_TESTING )
  enable_testing()
  add_subdirectory( test )
endif( BUILD_TESTING )

