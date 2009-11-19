/** 
 *  ==============================================================================
 * 
 *          \file   encoder.h
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-18 16:12:13.356033
 *  
 *   Description:  General class Encoder and Decoder
 *                 For specific encoder say NormalHuffEnoder can
 *                 derive from Encoder
 *  ==============================================================================
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdio.h>
#include <string>
#include <iostream>   //for debug
#include <fstream>    //for debug output to file

#include "buffer.h"      //fast file read write with buffer
#include "type_traits.h" //char_tag  TypeTraits

namespace glzip {

//---------------------------------------------------------------------Encoder
/**Here using traits instead of specializing*/
/**
 * For a encoder, it provide
 * 
 * caculate_frequency()
 * gen_encode()
 * write_encode_info()
 * encode_file()
 * print_encode()
 *
 * among those functions 
 * gen_encode() and write_encode_info()
 * must be implemented by the specific derived encoder class,
 * lik HuffEncoder or CanonicalEncoder
 *
 * The derived encoder also provide the final outfile name.
 *
 */
template<typename _KeyType>
class Encoder {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
public:
  Encoder(const std::string& infile_name, std::string& outfile_name) 
    : infile_name_(infile_name){
    infile_ = fopen(infile_name.c_str(), "rb");
    do_init(type_catergory());
  }
  virtual ~Encoder() {
    //std::cout << "destuct encoder\n";
    fclose(infile_);
    fclose(outfile_);
  }
  void caculate_frequency() {
    do_caculate_frequency(type_catergory());
  }
  
  virtual void gen_encode() = 0;  //must be implemented by specific encoder

  virtual void write_encode_info() = 0;
  
  ///对整个文件编码
  void encode_file() {
    do_encode_file(type_catergory());
  }

  ///打印huffman 压缩过程中 字符编码表及相关统计数据
  void print_encode(std::ostream& out = std::cout) {
    do_print_encode(type_catergory(), out);
  }

  void print_encode_length(std::ostream& out = std::cout) {
    //write the encoding length info to a separate file for conving that the canonical huff encoder
    //has correctly cacluate the encoding length just as the norma huff encoder
    for (int i = 0 ; i < 256 ; i++) {
      out << encode_map_[i].length() << std::endl;
    }
  }

private:
  void do_print_encode(char_tag, std::ostream& out);
  
  void do_init(char_tag) { 
    //for char we use array for frequence map, and vector<string> 
    //for encode map,we have to init them.
    for (int i = 0; i < 256; i++)
      frequency_map_[i] = 0;
    encode_map_.resize(256);  
  }
  
  //for key type is unsigned char
  //TODO if using iterator than for char and string we can use the same func
  void do_caculate_frequency(char_tag) {
    Buffer reader(infile_);  
    unsigned char key;
    while(reader.read_byte(key)) 
      frequency_map_[key] += 1;
    //std::cout << "Finished caculating frequency\n"; 
  }

  void do_encode_file(char_tag);

  //---------------------------------------for word(string) based below
  void init_help(string_tag){}


  void do_caculate_frequency(string_tag) {
  }

  void do_encode_file(string_tag) {

  }
protected:
  FILE*                 infile_;
  FILE*                 outfile_;
  EncodeHashMap         encode_map_;
  FrequencyHashMap      frequency_map_;

  const std::string&    infile_name_;        //for debug gen_enocde print log 

};

//---------------------------------------------------------------------Decoder
template<typename _KeyType>
class Decoder {
public:
  Decoder(const std::string& infile_name, std::string& outfile_name) {
    infile_ = fopen(infile_name.c_str(), "rb");
    if (outfile_name.empty())
      outfile_name = infile_name + ".de";
    outfile_ = fopen(outfile_name.c_str(), "wb");
  }
  ~Decoder() {
    //std::cout << "destruct decoder\n";
    fclose(infile_);
    fclose(outfile_);
  }
  
  virtual void get_encode_info() = 0; 
  virtual void decode_file() = 0;
protected:
  FILE*   infile_;
  FILE*   outfile_;
};

}  //----end of namespace glzip
#ifndef ENCODER_CC_
#include "encoder.cc"
#endif

#endif  //----end of ENCODER_H_
