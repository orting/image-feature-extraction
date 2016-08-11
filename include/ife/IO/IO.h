#ifndef __IO_h
#define __IO_h

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <limits>

#include "ife/Util/String.h"

typedef std::pair< std::string, std::string > StringPair;


template<typename CharT, typename ElemT>
ElemT
parseElementFromString(const std::basic_string<CharT>& s) {
  ElemT e;
  std::basic_istringstream< CharT >(s) >> e;
  return e;
}


template< typename InputIt >
std::ostream&
writeSequenceAsText( std::ostream& out,
		     InputIt begin,
		     InputIt end,
		     char sep=',' ) {
  bool first = true;
  while ( begin != end ) {
    if ( first ) {
      first = false;
    }
    else { 
      out << sep;
    }
    out  << *begin++;
  }
  return out;
}

// template< typename InputIt, typename CharT >
// void
// writeSequenceOfSequenceAsText( std::ostream& out,
// 			       InputIt begin,
// 			       InputIt end,
// 			       CharT outer_sep='\n',
// 			       CharT inner_sep=',' ) {
//   bool first = true;
//   while ( begin != end ) {
//     if ( !first ) {
//       out << outer_sep;
//     }
//     writeSequenceAsText( out, begin->begin(), begin->end() );
//     first = false;
//     ++begin;
//   }
// }


template< typename ElemT, typename CharT, typename OutputIt >
void
readTextSequence( std::istream& is,
		  OutputIt out,
		  CharT sep=',' ) {
  const std::streamsize count = std::numeric_limits<std::streamsize>::max();
  std::basic_string< CharT > s;
  ElemT elem;
  while ( (is >> elem) ) {
    *out++ = elem;
    is.ignore( count, sep );
  }
}


template< typename ElemT, typename CharT, typename OutputIt >
std::pair<size_t, size_t>
readTextMatrix( std::istream& is,
		OutputIt out,
		CharT colSep=',',
		CharT rowSep='\n' ) {
  size_t rows = 0;
  size_t cols = 0;
  std::basic_string< CharT > row;
  while ( is.good() ) {
    // Read a row and split into columns
    std::getline( is, row, rowSep );
    auto elements = split( row, colSep );
    if ( elements.size() > 0 ) {
      // We use the number of elements in the first row as the number of columns
      // and assert that following rows have the same sumber of columns
      if ( rows == 0 ) {
	cols = elements.size();
      }
      assert( elements.size() == cols );
      
      // Now we iterate over the elements, convert to the proper type and store
      // in out
      for ( const auto& element : elements ) {
	*out++ = parseElementFromString<CharT,ElemT>(element);
      }
      ++rows;
    }
  }
  return std::make_pair(rows, cols);
}




std::vector< StringPair >
readPairList( std::string inPath, char sep=',' );






#endif
