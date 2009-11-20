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

#ifdef DEBUG
#include <gtest/gtest.h>
#endif

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
//
//TODO TODO! Right now not consider encoding length > 32 EXPECT_LE(length, 32)
//now using int not unsiged int so not exceeding 31
template<>
class CanonicalHuffEncoder<unsigned char> : public Encoder<unsigned char> {
private:
  //---------------------prepare for the priority queue
  typedef unsigned char KeyType;
  typedef unsigned char _KeyType;
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
    std::string postfix(".crs2");
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
    do_gen_encode(length_, 256);
  }

  /**
   * input: 
   * length array container, and size
   * l is the lenth array, and the length array size n
   * ie. gen_encode(length_, 256)
   *     gen_encode(vec, vec.size())
   *     vec must support random access
   *function:
   * caculate the encode for each symbol
   * and also get first_code array --- the start code for each length
   * For simplicity write the encode info to the head of
   * the compressed file
   */
  //:) good for code reuse both char and string
  // orignal length array   
  // 2 3 3 2 3 1
  // after sort Bucket sorting! Not std::sort! 
  // 1 2 2 3 3 3
  // num array
  // length       1  2  3   
  // num          1  2  3
  // start_pos    0  1  3
  // symbol array 5 0 3 1 2 4  ---- index for the key 
  //              bucket sorting but using index as sorting result
  //              **TODO** becuase n  the number of symbols
  //              is much larger than max_length the number of 
  //              different lengths
  // coding 3    000
  //        3    001
  //        3    010
  //        2    10
  //        2    11
  //        1    ..     //TODO  prove will not occour this case
  //  In the book 
  //  first_code[l] <- (first_code[l+1] + num[l+1])/2
  //  But is it always ok? for eample 333 22?
  //  first_code[l] <- (first_code[l+1] + num[l+1] - 1)/2 + 1
  //  I think is always ok,how to prove? TODO TODO
  template<typename Container>
  void do_gen_encode(Container &l, int n) {
    
    //note length array input from first --- last , 0--n-1
    //for array of size max_length + 1 ,will index from 1
    int max_length = *std::max_element(&l[0], &l[n]);
#ifdef DEBUG
    int max_length2 = 0;
    for (int i = 0; i < n; i++) {
      if (l[i] > max_length2)
        max_length2 = l[i];
    }
    EXPECT_EQ(max_length2, max_length);
#endif  
    int array_len = max_length + 1;
    int symbol[n];        // sorted array of character key map index
    int num[array_len];  //0 is not used,num[1] hold in length array the length 1 num 
    int start_pos[array_len];  //for buckt sorting
    int first_code[array_len]; //the start encode for each length
    int next_code[array_len];  //for set encode

    //-----------------init  o(max_length)
    for (int i = 1; i <= max_length; i++) {
      num[i] = 0;
      start_pos[i] = 0;
    }
    
    //------------------caclc each length num  o(n) n 
    for (int i = 0; i < n; i++) {  //range n length of the length array
      num[l[i]] += 1; 
      symbol[i] = -1;
    }
    
    //--------------caclc the start postion for each length  o(max_length)
    for (int i = 1; i <= max_length; i++) {
      if (num[i]) {
        start_pos[i] = num[i - 1] + start_pos[i - 1];
      }
    }
    
    //------------------calc first code for each length  o(max_length)
    //next_code now is a copy of first_code
    first_code[max_length] = 0;
    next_code[max_length] = 0;
    for (int i = max_length - 1; i >= 1; i--) {
      first_code[i] = (first_code[i + 1] + num[i + 1]) / 2;
      next_code[i] = first_code[i];
    }
    
    //------------------OK now can write the header out
    //we need to write the 
    //start_pos  (max_length) , 
    //first_code (max_length) 
    //symbol_    (n)
    do_write_encode_info(symbol, n, start_pos, first_code, max_length);

    //-------------------write the encode info since 

    //------------------finish encoding for each symbol and calc symbol array
    //TODO can not be reused for word based hash??
    for (int i = 0; i < n; i++) {
      int len = l[i];
      if (len) {  //the caracter really exists for cha 256
        encode_map_[i] = next_code[len]++;  //TODO pass encode map?
        symbol[start_pos[len]++] = i;   //bucket sorting --using index
      }
    }
    //or can use symbol_[start_pos[len] + next_code[len] - first_code[len]]
    //so not modify start_pos array ,since n is big we use start_pos[len]++
#ifdef DEBUG
    std::string outfile_name = this->infile_name_ + "_canonical_encode.log";
    std::cout << "Writting canonical encode info to " << outfile_name << std::endl;
    std::ofstream out_file(outfile_name.c_str());
    print_encode(symbol, n, out_file);
#endif 
  }

  void print_encode(int symbol[], int n, std::ostream& out = std::cout)  
  {
    using namespace std;
   //-----打印编码信息
    out << "The canonical huffman encoding map:\n\n";
    
    out << setiosflags(ios::left) << setw(20) << "Character"
                                  << setw(20) << "Times"
                                  << setw(20) << "EncodeLength"
                                  << setw(20) << "encode_map_[i]"
                                  << setw(30) << "Encode" << "\n\n";
    for (int i = 0; i < n; i++) {
      if (symbol[i] != -1) {
        //左对齐,占位
        unsigned char key = symbol[i];
        int j = symbol[i];
        std::string code;
        get_encode_string(j, code);
        if (key == '\n') {
         out << setiosflags(ios::left) << setw(20) << "\\n"  
                                << setw(20) << frequency_map_[j]
                                << setw(20) << length_[j]
                                << setw(20) << encode_map_[j]
                                << setw(30) << code << "\n";
        } else if (key == ' ') {
          out << setiosflags(ios::left) << setw(20) << "space" 
                                << setw(20) << frequency_map_[j]
                                << setw(20) << length_[j]
                                << setw(20) << encode_map_[j]
                                << setw(30) << code << "\n";
        } else {
          out << setiosflags(ios::left) << setw(20) << key
                                << setw(20) << frequency_map_[j]
                                << setw(20) << length_[j]
                                << setw(20) << encode_map_[j]
                                << setw(30) << code << "\n";
        }
      }
    }

  }

  ///get the encode string of index i(the symbol unsigned char(i))
  void get_encode_string(int i, std::string& s) 
  {
    unsigned int code = encode_map_[i];
    unsigned int len = length_[i];
    unsigned int mask = 1 << (len - 1);
    for (int i = 0; i < len; i++) {
      if ((code & mask) == 0) {
        s.push_back('0');
      } else {
        s.push_back('1');
      }
      mask >>= 1;
    }
  }
  ///write header
  //TODO for word based method n is big 
  //will not compress well? since header is big?
  void do_write_encode_info(int symbol[], int n,
      int start_pos[], int first_code[], int max_len) 
  {
    Buffer writer(outfile_);
    
    writer.write_byte(n);      //n symbols
    for (int i = 0; i< n; i++) 
      writer.write_byte(symbol[i]);
    //write from index 1
    writer.write_byte(max_len); 
    for (int i = 0; i < max_len; i++) {
      writer.write_byte(start_pos[i]);
      writer.write_byte(first_code[i]);
    }
    
    writer.flush_buf();
  }
  
  ///empty already done in gen_encode()
  void write_encode_info() {
  }

private:
  
  virtual void encode_each_byte(Buffer &reader, Buffer &writer) 
  {
    unsigned char key;
    while(reader.read_byte(key)) 
      writer.write_bits(encode_map_[key], length_[key]);
  }
  ///cacluate each symbol's enoding length,will be the same as normal huffman
  void get_encoding_length();
private: 
  int length_[256];  //store encode length lenghth_ do not support .size() :(

  //int symbol_[256];  //n is 256 for char
  int encode_map_[256];  //do not use encode_map_  in the base TODO TODO unsinged int?
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
