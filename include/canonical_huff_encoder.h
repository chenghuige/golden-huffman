/** 
 *  ==============================================================================
 * 
 *          \file   canonical_huff_encoder.h
 *
 *        \author   pku_goldenlock@qq.com
 *                  ChengHuige
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

#ifdef DEBUG
#include <gtest/gtest.h>
#endif
#include <bitset>

namespace glzip {

//Well, I have tested that ifstream.rdbuf and istreambuf_iterator
//will perfom as well as the Buffer class I have wrote, so it is 
//better to use what have been in the stl libaray, but I will not
//modify since the work has been done :( and the performance differ little
//Also I have wrote iterator support both interanl and out but the performance
//is a bit lower. 0.2 ms slow than istreambuf_iterator for 240M file,so not 
//a big problem.

//TODO TODO! Right now not consider encoding length > 32 EXPECT_LE(length, 32)
//now using int not unsiged int so not exceeding 31

template<typename _KeyType = unsigned char>
class CanonicalHuffEncoder: public Encoder<_KeyType> {
private:
  //---------------------prepare for the priority queue
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
  
  typedef Encoder<_KeyType>        Base;
  using Base::infile_;
  using Base::outfile_;
  using Base::frequency_map_;

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
      : Encoder<unsigned char>(infile_name) {
    set_out_file(infile_name, outfile_name);
  }

  CanonicalHuffEncoder() {}

  void set_out_file(const std::string& infile_name, std::string& outfile_name);
  void set_file(const std::string& infile_name, std::string& outfile_name);
  
  void gen_encode();

  /**write header*/
  void write_encode_info();

  void encode_file();
private:
  
  //this is used during Encoder::encode_file()
  void encode_each_byte(Buffer &writer);
  /**cacluate each symbol's enoding length,will be the same as normal huffman*/

  void get_encoding_length();
  
  template<typename Container>
  void do_gen_encode(Container &l);

  void print_encode(unsigned int symbol[], int n, std::ostream& out = std::cout);
  /**get the encode string of index i(the symbol unsigned char(i))*/

  void get_encode_string(int i, std::string& s);

private: 
  /**encoding length array*/
  unsigned int length_[CharSymbolNum];    
  unsigned int codeword_[CharSymbolNum];   

  int max_len_; //max elem of length_[]
  int min_len_; //min encoding length

  /** like 0000 for max_encoding_length the min encoing value of each encoding length */
  unsigned int first_code_[64];      //max_encoding_length should <= 32, we need at most 32+1, now use 64
  
  /** the index position of the min encoing in symbol for each encoding length */
  unsigned int start_pos_[64];       
  
  /** the sorted array of symbol index like length_[symbol[0]] is min length.*/
  unsigned int symbol_[CharSymbolNum];           
};

//TODO right now unlike encoder decoder do not support reuse! set_file
//TODO tried to use policy to choose differnt dece

/**
 * This is a CanonicalHuffDecoder using the simplest decoding method
 */
template<typename _KeyType = unsigned char>
class CanonicalHuffDecoder : public Decoder<_KeyType> {
public:
  CanonicalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<_KeyType>(infile_name, outfile_name), reader_(this->infile_), writer_(this->outfile_){}

  //read header init all symbol_[] start_pos_[] first_code_[]
  void get_encode_info();

  //will use reader_ and writer_
  void decode_file();
protected:
  unsigned int symbol_[CharSymbolNum];         //symbol less than 256
  unsigned int start_pos_[64];       //encoding max length less than 32 TODO may > 32?
  unsigned int first_code_[64];
  
  unsigned int min_len_;
  unsigned int max_len_;       //max encoding length

  Buffer  reader_;
  Buffer  writer_;          
};

namespace canonical_help {
 //5 4 3 2 1 to search 3.5 will return the pos poiting to 3, the first val 3.5 >= 
  //search 3 will return the same pos
  //linear search
#ifdef DEBUG
unsigned long long  search_times = 0;
#endif
  int cfind(unsigned int vec[], unsigned int start, unsigned int val) {
    while(val < vec[start]) {
#ifdef DEBUG
      search_times++;
#endif
      ++start;
    }
    return start;
  }
  //the same as find except for using binary search
  //int bfind(unsigned int vec[], int length) {
  //}
} 

template<typename _KeyType = unsigned char>
class FastCanonicalHuffDecoder : public CanonicalHuffDecoder<_KeyType> {
public:
  FastCanonicalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : CanonicalHuffDecoder<_KeyType>(infile_name, outfile_name){}
  
  typedef CanonicalHuffDecoder<_KeyType> Base;
  using   Base::reader_;
  using   Base::writer_;
  using   Base::first_code_;
  using   Base::start_pos_;
  using   Base::symbol_;
  using   Base::max_len_;
  using   Base::min_len_;
  //TODO using canonical_help::cfind; //wrong!

   void decode_file();
};

template<typename _KeyType = unsigned char, int TableLength = 8>
class TableCanonicalHuffDecoder : public CanonicalHuffDecoder<_KeyType> {
public:
  TableCanonicalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : CanonicalHuffDecoder<_KeyType>(infile_name, outfile_name){}
  
  typedef CanonicalHuffDecoder<_KeyType> Base;
  using   Base::reader_;
  using   Base::writer_;
  using   Base::first_code_;
  using   Base::start_pos_;
  using   Base::symbol_;
  using   Base::max_len_;
  using   Base::min_len_;
  
  //the set up lookuptable cost will be o(n) n is symbol num
  //it must be called before firest_code_[] is left justified
  void setup_lookup_table();
  
  void decode_file();

private:
  unsigned int lookup_table_[1 << TableLength];  //now be 2^8 256
};
//------------------------------------------------------------------------------------------

}  //----end of namespace glzip
#ifndef CANONICAL_HUFF_ENCODER_CC_
#include "canonical_huff_encoder.cc"
#endif

#endif  //----end of CANONICAL_HUFF_ENCODER_H_
