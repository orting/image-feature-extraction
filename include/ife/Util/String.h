#ifndef __String_h
#define __String_h

#include <vector>
#include <string>

/**
   \brief Functions for working with strings
*/


/**
 * Remove characters from begining and end of string
 * \param s      The string that should be trimmed
 * \param chars  A string containing the characters that should be removed 
 *               from beginning and end of s.
 * \return       s with chars stripped from begining and end
 */
std::string
trim(std::string s, std::string chars=" ");
// {
//   auto a = s.find_first_not_of(chars);
//   auto b = s.find_last_not_of(chars);
//   return s.substr(a, b-a+1);
// }


/**
 * Tokenize a string on given delimiter
 * \param s      The string to tokenize
 * \param delim  The character to tokenize s on
 * \return       A vector of string tokens
 */
std::vector< std::string > 
split(std::string s, char delim);
// {
//   std::vector< std::string > tokens;
//   std::size_t start = 0;
//   for (std::size_t end = start; end < s.size(); ++end) {
//     if (s[end] == delim) {
//       tokens.push_back(s.substr(start, end-start));
//       start = end + 1;
//     }
//   }
//   if (start < s.size()) {
//     tokens.push_back(s.substr(start, s.npos));
//   }
//   return tokens;
// }

#endif
