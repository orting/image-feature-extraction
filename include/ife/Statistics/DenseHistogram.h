#ifndef __DenseHistogram_h
#define __DenseHistogram_h

#include <algorithm>
#include <iterator>
#include <vector>
#include <limits>

#include "ife/IO/IO.h"

template< typename NumType >
class DenseHistogram {
public:
  typedef NumType value_type;
  typedef typename std::vector< value_type >::size_type size_type;

  template< typename T >
  friend std::ostream& operator<<(std::ostream&,
				  const DenseHistogram<T>&);

  /*
    DenseHistogram needs a collection of sorted edges indicating how the 
    histogram should be generated.
    The bins are defined as 
    (-inf, edges[0]], (edges[0], edges[1]], ..., (edges[n-1], inf)
  */
  template< typename InputIt >
  DenseHistogram( InputIt begin, InputIt end )
    : m_Edges( begin, end ), m_Counts( m_Edges.size() + 1 )
  {
    assert( m_Edges.size() > 0 );
    assert( m_Counts.size() > 1 );
  }

  
  // edges should be sorted
  DenseHistogram( std::initializer_list< value_type > edges )
    : m_Edges( edges ), m_Counts( edges.size() + 1 )
  {
    assert( m_Edges.size() > 0 );
    assert( m_Counts.size() > 1 );
  }

  // Insert value in bin such that value is greater than the left edge and
  // less than or equal to the right edge.
  void insert( value_type value ) {
    auto edge = std::lower_bound(m_Edges.begin(), m_Edges.end(), value );
    auto bin = std::distance( m_Edges.begin(), edge );
    assert( bin >= 0 );
    assert( bin < m_Counts.size() );
    ++m_Counts[bin];
  }

  std::vector< value_type> getFrequencies() {
    std::vector< value_type > frequencies( m_Counts.size() );
    value_type sum = std::accumulate(m_Counts.begin(), m_Counts.end(), 0);
    std::transform(m_Counts.begin(), m_Counts.end(), frequencies.begin(), [sum](value_type c){ return c/sum; });
    return frequencies;
  }  

  std::vector<unsigned int> getCounts() {
    return m_Counts;
  }

  void resetCounts() {
    std::fill(m_Counts.begin(), m_Counts.end(), 0);
  }

  std::size_t getNumberOfBins() const {
    return m_Counts.size();
  }
  
private:
  std::vector< value_type > m_Edges;
  std::vector< unsigned int > m_Counts;
  size_type m_Center, m_High;  
};

template<typename T>
std::ostream&
operator<<( std::ostream& os, const DenseHistogram< T >& hist ) {
  return writeSequenceAsText( os, hist.m_Counts.begin(), hist.m_Counts.end() );
}

#endif
