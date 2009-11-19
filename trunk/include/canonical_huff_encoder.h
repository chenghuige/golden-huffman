/** 
 *  ==============================================================================
 * 
 *          \file   canonical_huff_encoder.h
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-18 16:36:29.568105
 *  
 *   Description:   Class of CanonicalHuffEncoder and CanonicalHuffDecoder
 *                  
 *                  Will use canonical huffman method for compressing 
 *                  and decompressing
 *  ==============================================================================
 */

#ifndef CANONICAL_HUFF_ENCODER_H_
#define CANONICAL_HUFF_ENCODER_H_

#include "encoder.h"
#include <string>
#include <queue>
#include <deque>
#include <vector>
#include <functional>

namespace glzip {

/*
 *  Since it for char type it is not use too much mem,only at most 256 characters
 *  Will use different method for char type and word/string type.
 *  Here using template specialization
 */

//The version for std::string or char *
template<typename _KeyType>
class CanonicalHuffEncoder : public Encoder<_KeyType> {

};


//The version for unsigned char
//Since not consuming much mem, use as simple method as possible
//So can simply use the huftree, here however sitll not use it 
//but using two array index and length to moni huftree to get 
//encoding length
//TODO if file is large when adding in the priority queue
//a1 + a2 ..... what if exceeds  long long??TODO
//TODO what if we use float of grequence instead of long long
//will be slower? How much?
//TODO for frequency_map_ actually it is useless after we get length
//but since 256 is small it's ok if big num than we shoud free the mem
template<>
class CanonicalHuffEncoder<unsigned char> : public Encoder<unsigned char> {
private:
  //---------------------prepare for the priority queue
  typedef unsigned char KeyType;
  typedef TypeTraits<KeyType>::FrequencyHashMap        FrequencyHashMap;

  struct HuffNodeIndexGreater:
      public std::binary_function<int, int, bool> {
    explicit HuffNodeIndexGreater(FrequencyHashMap& frequency_map)
      :f_map_(frequency_map) {}
    bool operator() (const int index1, const int index2) {
      return f_map_[index1] > f_map_[index2];
    }
  private:
  FrequencyHashMap& f_map_;  
  };
  
  typedef std::deque<int> HuffDQU;   
  typedef std::priority_queue<int, HuffDQU, HuffNodeIndexGreater>  HuffPRQUE; 


public:  
  CanonicalHuffEncoder(const std::string& infile_name, std::string& outfile_name) 
      : Encoder<unsigned char>(infile_name, outfile_name) {
    set_out_file(infile_name, outfile_name);
  }

  CanonicalHuffEncoder() {}

  void set_out_file(const std::string& infile_name, std::string& outfile_name) {
    std::string postfix = ".crs2";                                 
    if (outfile_name.empty())
      outfile_name = infile_name + postfix;
    this->outfile_ = fopen(outfile_name.c_str(), "wb");
  }

  void set_file(const std::string& infile_name, std::string& outfile_name) {
    Encoder<unsigned char>::set_file(infile_name, outfile_name);
    set_out_file(infile_name, outfile_name);
  }

  void gen_encode() {
    get_encoding_length();
  }

  void write_encode_info() {

  }

private:
  void get_encoding_length();
private: 
  int length_[256];  //store encode length
};





template<typename _KeyType>
class CanonicalHuffDecoder : public Decoder<_KeyType> {
public:

};

//------------------------------------------------------------------------------------------

}  //----end of namespace glzip
#ifndef CANONICAL_HUFF_ENCODER_CC_
#include "canonical_huff_encoder.cc"
#endif

#endif  //----end of CANONICAL_HUFF_ENCODER_H_
