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
#include <bitset>

namespace glzip {

/*
 *  Since it for char type it is not use too much mem,only at most 256 characters
 *  Will use different method for char type and word/string type.
 *  Here using template specialization
 */

  //TODO!! again try to use ifstream.rdbuf()!insdead of buffer!
  //try to use iterator! See the performance!

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
      : Encoder<unsigned char>(infile_name) {
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
    this->set_infile(infile_name);
    set_out_file(infile_name, outfile_name);
  }

  void gen_encode() {
    //set up length_
    get_encoding_length();
    //gen encode from length_ an write header at last step
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
  void do_gen_encode(Container &l, int n)
  {
    
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
    int array_len = max_length + 1;  //+1!!
    int symbol[n];        // sorted array of character key map index
    int num[array_len];  //0 is not used,num[1] hold in length array the length 1 num 
    int start_pos[array_len];  //for buckt sorting
    int start_pos_copy[array_len];
    int first_code[array_len]; //the start encode for each length
    int next_code[array_len];  //for set encode

    //-----------------init  o(max_length)
    for (int i = 0; i <= max_length; i++) {
      num[i] = 0;
      start_pos[i] = 0;
    }
    
    //------------------caclc each length num  o(n) n 
    for (int i = 0; i < n; i++) {  //range n length of the length array
      num[l[i]] += 1; 
      symbol[i] = -1;
    }
    num[0] = 0;  //for num[0] may be added since l[i] can be 0
    
    //--------------caclc the start postion for each length  o(max_length)
    for (int i = 1; i <= max_length; i++) 
      start_pos[i] = num[i - 1] + start_pos[i - 1];
    
    //------------------calc first code for each length  o(max_length)
    //next_code now is a copy of first_code
    first_code[max_length] = 0;
    next_code[max_length] = 0;
    for (int i = max_length - 1; i >= 1; i--) {
      first_code[i] = (first_code[i + 1] + num[i + 1]) / 2;
      next_code[i] = first_code[i];
    }

    //-------------------write the encode info since 

    //------------------finish encoding for each symbol and calc symbol array
    //TODO can not be reused for word based hash??
    //or can use symbol_[start_pos[len] + next_code[len] - first_code[len]]
    //so not modify start_pos array and do not need start_pos_copy[],
    //but since n is big we use start_pos[len]++

    //save
    std::copy(start_pos, start_pos + array_len, start_pos_copy);
   
    for (int i = 0; i < n; i++) {
      int len = l[i];
      if (len) {  //the caracter really exists for cha 256 l[i] > 0
        codeword_[i] = next_code[len]++;  //TODO pass encode map?
        symbol[start_pos[len]++] = i;   //bucket sorting --using index
      }
    }

    //------------------------------------------------OK now can write the header out!
    //we need to write the 
    //start_pos  (max_length) ,  we use start_pos_copy which is correct unmodified
    //first_code (max_length) 
    //symbol_    (n)
    do_write_encode_info(symbol, n, start_pos_copy, first_code, max_length);

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
                                  << setw(20) << "codeword_[i]"
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
                                << setw(20) << codeword_[j]
                                << setw(30) << code << "\n";
        } else if (key == ' ') {
          out << setiosflags(ios::left) << setw(20) << "space" 
                                << setw(20) << frequency_map_[j]
                                << setw(20) << length_[j]
                                << setw(20) << codeword_[j]
                                << setw(30) << code << "\n";
        } else {
          out << setiosflags(ios::left) << setw(20) << key
                                << setw(20) << frequency_map_[j]
                                << setw(20) << length_[j]
                                << setw(20) << codeword_[j]
                                << setw(30) << code << "\n";
        }
      }
    }

  }

  /**get the encode string of index i(the symbol unsigned char(i))*/
  void get_encode_string(int i, std::string& s) 
  {
    unsigned int code = codeword_[i];
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

  /**write header*/
  //TODO for word based method n is big 
  //will not compress well? since header is big?
  void do_write_encode_info(int symbol[], int n,
      int start_pos[], int first_code[], int max_length) 
  {
    Buffer writer(outfile_);

    //TODO!! right now its is ok since 256 symbols at most!
    //But if using word the symbols is large
    //So need at least a int type 4 bytes!
    //how to write in n using writ_byte bitset will ok,better way?
    //FIXME
#ifdef DEBUG
    std::cout << "write n is " << n << std::endl;
#endif
    writer.write_byte(n);      //n symbols
    //FIXME
    for (int i = 0; i< n; i++) 
      writer.write_byte(symbol[i]);

    //write from index 1
#ifdef DEBUG
    std::cout << "write max encoding length is " << max_length << std::endl;
#endif
    writer.write_byte(max_length); 
    for (int i = 1; i <= max_length; i++) { //notice start from 1 fixed!!!!
      writer.write_byte(start_pos[i]);
      writer.write_byte(first_code[i]);
    }
    writer.flush_buf();
  }
  
  ///empty since already done in gen_encode() TODO better arrange class Compressor.compress()?
  void write_encode_info() {
  }

private:
  
  //this is used during Encoder::encode_file()
  virtual void encode_each_byte(Buffer &reader, Buffer &writer) 
  {
    unsigned char key;
    while(reader.read_byte(key)) 
      //what we provide is encode in int type and encode length
      writer.write_bits(codeword_[key], length_[key]);
  }

  /**cacluate each symbol's enoding length,will be the same as normal huffman*/
  void get_encoding_length();
private: 
  int length_[256];  //store encode length lenghth_ do not support .size() :(
  int codeword_[256];  //do not use encode_map_  in the base TODO TODO unsinged int?
};


//TODO right now unlike encoder decoder do not support reuse! set_file
template<typename _KeyType>
class CanonicalHuffDecoder : public Decoder<_KeyType> {
public:
  CanonicalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<_KeyType>(infile_name, outfile_name), reader_(this->infile_){}

  //read header init all symbol_[] start_pos_[] first_code_[]
  void get_encode_info() 
  {
    //TODO to be modified as reading 4 bytes int not one byte!
    //FIXME
    int n;                //symbol num
    //unsigned char n;
    reader_.read_byte(n);
    //FIXME
    n = 256;            //now for char 256
    symbol_.resize(n);
#ifdef DEBUG
    std::cout << "symbol num is " << (int)n << std::endl;
#endif
       
    //TODO symbol array for word based method must use at least int type
    //FIXME
    for (int i = 0; i < n; i++) 
      reader_.read_byte(symbol_[i]);
    
    int max_length;
    reader_.read_byte(max_length);   //max encoding length
    start_pos_.resize(max_length + 1);  //notice 0 is unused,so max_length + 1 is th size!!!!fixed
    first_code_.resize(max_length + 1);

#ifdef DEBUG
    std::cout << "max encoding length is " << max_length << std::endl;
#endif

      
    for (int i = 1; i <= max_length; i++) {  //from 1!
      reader_.read_byte(start_pos_[i]);
      reader_.read_byte(first_code_[i]);
    } 
  }

  //TODO decode_file can share the same frame work
  //so can be in the base Decoder,and using virtual decode_byte
  //but 2 problems will occour
  //1. decode_byte be virtual function may affect speed 
  //2. reader_ has to be in the base and carefully passed
  //TODO now a little code copy, if we change the
  //store method sequence in the Encoder.encode_file
  //Then NormalHuffDecoder.decode_file using huff_tree
  //and here CanonicalHuffDecoder all must change:(
  //FIXME better structure your code
  //may be decode_file(Buffer &reader)
  //or decode_file(Buffer &reader, Buffer &writer);
  //HuffTree use the reader_ passed by Decoder not pass infile_
  void decode_file() 
  {
    Buffer writer(this->outfile_);
    unsigned char left_bit, last_byte;
    reader_.read_byte(left_bit);
    reader_.read_byte(last_byte);
    //--------------------------------------decode each byte
    unsigned char c;
    
    int v = 0;        //like a global status for value right now
    int len = 0;      //like a global status for length
    while(reader_.read_byte(c)) 
      decode_byte(c, writer, v, len);
    //--------------------------------------deal with the last byte
    if (left_bit)
      decode_byte(last_byte, writer, v, len, (8 - left_bit));
    writer.flush_buf();
    fflush(this->outfile_);
  }

private:

  //FIXME may be not start from 1 bit but the min_encoding_length
  //carefully deal with 22 444  withou length1, 2, cases like this
  //TODO really use read bit? speed?
  void decode_byte(unsigned char c, Buffer& writer, int &v, int &len, int bit_num = 8)
  {
    std::bitset<8> bits(c);
    int end = 7 - bit_num;
    for (int i = 7; i > end; i--) {
      v = v * 2 + bits[i];  //TODO * and << which is quicker?
      len++;                //length add 1
      if (v >= first_code_[len]) {  //OK in length len we translate one symbol
        //TODO for word based how ?
        //std::cout << len << " " << v << " " << start_pos_[len] << std::endl;
        //std::cout << symbol_[start_pos_[len] + v - first_code_[len]] << std::endl;
        writer.write_byte(symbol_[start_pos_[len] + v - first_code_[len]]);
        v = 0;
        len = 0;        //fished one translation set to 0
      }
    }
  }

private:
  //TODO see the performance comparing with int []!
  //if we use int [] than we must get the length in get_encode_info, then
  //decode_file must be in get_encode_info
  //like at last of get_encode_info()
  //decode_file(int symbol[], int start_pos[])
  //, other wise we can not share data!
  std::vector<int>  symbol_;       //size n   index for key
  std::vector<int>  start_pos_;    //size max_encoding_length index for symbol
  std::vector<int>  first_code_;
  Buffer            reader_;
};


//------------------------------------------------------------------------------------------

}  //----end of namespace glzip
#ifndef CANONICAL_HUFF_ENCODER_CC_
#include "canonical_huff_encoder.cc"
#endif

#endif  //----end of CANONICAL_HUFF_ENCODER_H_
