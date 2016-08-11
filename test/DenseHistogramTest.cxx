/*
  Test the dense histogram class
 */
#include "gtest/gtest.h"

#include "ife/Statistics/DenseHistogram.h"

typedef float RealType;

TEST( DenseHistogram, Counts ) {
  std::vector< RealType > values{ -1, 0, 0.5, 1,
		      	         1.5, 2.1,
		      	         2.6, 2.9,
		      	         3.2, 3.5, 4.2, 4.6,
		      	         5, 6,
		      	         7, 8,
		      	         9, 10
      };

  DenseHistogram<RealType> hist( { 1, 2.5, 3.0, 4.7, 6.2, 8.3 } );
  for ( auto value : values ) {
    hist.insert( value );
  }

  std::vector< unsigned int > expected{ 4, 2, 2, 4, 2, 2, 2 };
  auto actual = hist.getCounts();
  ASSERT_EQ( 7, actual.size() );
  for ( size_t i = 0; i < 7; ++i ) {
    EXPECT_EQ( expected[i], actual[i] );
  }
}

TEST( DenseHistogram, Frequencies ) {
  std::vector< RealType > values{ -1, 0, 0.5, 1,
		      	         1.5, 2.1,
		      	         2.6, 2.9,
		      	         3.2, 3.5, 4.2, 4.6,
		      	         5, 6,
		      	         7, 8,
		      	         9, 10
      };

  DenseHistogram<RealType> hist( { 1, 2.5, 3.0, 4.7, 6.2, 8.3 } );
  for ( auto value : values ) {
    hist.insert( value );
  }

  std::vector< RealType > expected{ 4.0/18, 2.0/18, 2.0/18, 4.0/18,
                                    2.0/18, 2.0/18, 2.0/18 };
  auto actual = hist.getFrequencies();
  ASSERT_EQ( 7, actual.size() );
  for ( size_t i = 0; i < 7; ++i ) {
    EXPECT_FLOAT_EQ( expected[i], actual[i] );
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
