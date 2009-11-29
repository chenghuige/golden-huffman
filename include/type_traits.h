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
#define CharSymbolNum 257
template<>
class TypeTraits<unsigned char> {
public:
  typedef char_tag type_catergory;
  typedef long long FrequencyHashMap[CharSymbolNum];        //use 256 + 1,1 is the end of encoding mark
  typedef FrequencyHashMap  HashMap;
  typedef std::vector<std::string>    EncodeHashMap;
};
//---special TypeTraits for std::string, word based encoding
//--------------------std::string and char * are two possible implementation choice for word based encoding
struct SymbolInfo {
  //we will reuse the memory of frequency to length!
  union{
    unsigned int frequency;       //FIXME do not exceed 2^32 right now, may be enough
    unsigned int length;
  }fl;
  int codeword;                 //encoding ie, 0110 and length = 4 will store here 6
  //for easy use in Tokenizer
  void operator+=(int i) {
    fl.frequency += i;
  }

  void operator=(int i) {
    fl.frequency = i;
  }
};

template<>
class TypeTraits<std::string> {
public:
  typedef string_tag type_catergory;
  
  //for word based huffman FrequencyHashMap also store other info besides frequency
  //0 for word, 1 for non word
  typedef std::tr1::unordered_map<std::string, SymbolInfo>  HashMap;
  typedef std::tr1::unordered_map<std::string, SymbolInfo>  FrequencyHashMap[2];
};



//template<>
//class TypeTraits<char *> {
//public:
//  typedef string_tag type_catergory;
//  typedef std::tr1::unordered_map<char *, size_t>   FrequencyHashMap;
//  typedef std::tr1::unordered_map<char *, char *>   EncodeHashMap;
//};

}   //end of namespace glzip
//-----------------------------------------------------------------------------

#endif  //end of TYPE_TRAITS_H
