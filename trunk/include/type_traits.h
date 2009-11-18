#ifndef _TYPE_TRAITS_H
#define _TYPE_TRAITS_H

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

struct encode_hufftree {};
struct decode_hufftree {};
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
  typedef long long count[256];
  typedef count     FrequencyHashMap;
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

#endif  //end of _TYPE_TRAITS_H
