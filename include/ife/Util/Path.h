#ifndef __Path_h
#define __Path_h

#include <string>

namespace Path {
  std::string join(std::string p1, std::string p2) {
    char sep = '/';
    auto p1_pos = p1.find_last_not_of( sep );
    p1_pos = p1_pos == std::string::npos
      ? 0           // Erase entire string
      : p1_pos + 1; // Preserve the last character
    p1.erase( p1_pos );
    p1.append( 1, sep );
    auto p2_pos = p2.find_first_not_of( sep );
    if ( p2_pos == std::string::npos ) {
      return p1;
    }
    p1.append( p2.substr( p2_pos ) );
    return p1;
  }
}

#endif
