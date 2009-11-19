/** 
 *  ==============================================================================
 * 
 *          \file   type_traits.h
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-18 16:12:13.356033
 *  
 *   Description:   Provide the all type info for Encoder and Decoder
 *                  especailly for dealing with both char based and word/string
 *                  based compressing, decompressing
 *  ==============================================================================
 */

#ifndef TYPE_TRAITS_H
#define TYPE_TRAITS_H

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <tr1::hash_map>
#endif
namespace std
{
 using namespace __gnu_cxx;
}
//#include<map>
#include<tr1/unordered_map>

namespace glzip{
//using namespace std;
typedef unsigned long long size_t;

//---------------------------------------------------------------------------
struct normal_tag {};
struct char_tag: public normal_tag {};    //for character based encoding
struct string_tag: public normal_tag {};  //for word based encoding

//----------------------------------------------------------------------------

//---TypeTraits,from here we can find the HashMap type 
template <typename _KeyType>
class TypeTraits {
public:
  typedef normal_tag  type_catergory;
  typedef std::tr1::unordered_map<_KeyType, size_t> HashMap;
};
//---special TypeTraits for unsigned char, character based encoding
template<>
class TypeTraits<unsigned char> {
public:
  typedef char_tag type_catergory;
  typedef long long FrequencyHashMap[256];
  typedef std::vector<std::string>    EncodeHashMap;
};
//---special TypeTraits for std::string, word based encoding
//--------------------std::string and char * are two possible implementation choice for word based encoding
template<>
class TypeTraits<std::string> {
public:
  typedef string_tag type_catergory;
  typedef std::tr1::unordered_map<std::string, size_t>        FrequencyHashMap;
  typedef std::tr1::unordered_map<std::string, std::string>   EncodeHashMap;
};

template<>
class TypeTraits<char *> {
public:
  typedef string_tag type_catergory;
  typedef std::tr1::unordered_map<char *, size_t>   FrequencyHashMap;
  typedef std::tr1::unordered_map<char *, char *>   EncodeHashMap;
};

}   //end of namespace glzip
//-----------------------------------------------------------------------------

#endif  //end of TYPE_TRAITS_H
