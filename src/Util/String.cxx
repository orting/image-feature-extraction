#include "ife/Util/String.h"

std::string
trim(std::string s, std::string chars) {
  auto a = s.find_first_not_of(chars);
  auto b = s.find_last_not_of(chars);
  return s.substr(a, b-a+1);
}


std::vector< std::string > 
split(std::string s, char delim) {
  std::vector< std::string > tokens;
  std::size_t start = 0;
  for (std::size_t end = start; end < s.size(); ++end) {
    if (s[end] == delim) {
      tokens.push_back(s.substr(start, end-start));
      start = end + 1;
    }
  }
  if (start < s.size()) {
    tokens.push_back(s.substr(start, s.npos));
  }
  return tokens;
}
