/** 
 *  \file compressor.h
 *  \author ChengHuige
 *  \date  2009-10-6
 */

#ifndef _COMPRESSOR_H_
#define _COMPRESSOR_H_

#include <stdio.h>
#include <string>
#include <iostream>   //for debug
#include <fstream>    //for debug output to file
#include <typeinfo>   //type id
#include <functional>
#include <numeric>
#include <iomanip>   //for setw 输出格式控制
#include "buffer.h"   //fast file read write with buffer
#include "huff_tree.h"
#include "type_traits.h"
/**
 * Compressor is a framework class,it can use any encoder to help compress work,
 * it accept infilename and outfilename(if not given will decide the oufilename)
 * it will compress the infile and outpu the outfile.
 * We can use HuffEncoder and CanonicalEnocder.
 * For each encoder we can choose the key type unsigned char or string
 * (unsigned char is character based,and string is word based).
 * Notice to use unsigned char instead of char,because we want to map to 0 - 255
 * Also in glzip 
 * typedef long long size_t; if using unsigned it it will be max 4G byte, I think that will be fine too.
 */
namespace glzip{

//------------------------------------------------------------------------------------------
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
  typedef typename TypeTraits<_KeyType>::type_catergory type_catergory;
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

  const std::string& infile_name_;        //for debug gen_enocde print log 

};

/*
 * When write_encode_info()
 * HuffEncoder will write the huff tree to the outfile first
 * using pre order travel  like intl, intl, leaf,......
 * each node using 2 bytes,the first one is 0 means it is leaf,
 * than the second one is the leaf key.
 * Other case it is a internal node.
 *
 * When encode_file()
 * To help dealing with the last byte,because at the end we might come up with
 * coding  1,1,0 not 8 bits, than we have to make it 1,1,0,  00000 than
 * write 11000000 as the last byte.
 * Will store the last byte(of encoding the file) before storing the encoded file.
 * Will use 2 bytes, the first byte tell how many bits are left ,so need to fill 0.
 * See for the case mentioned above.The second byte is the last byte encode,
 * like 1010000 mentiond above.
 * ie, Will store  0x05  0xc0 before store of encoding the file from begining.
 * meaning 5 bits are left(useless bits) and the last byte is 11000000, so we only
 * need to decode until finished 1,1,0 and ignore the last 5 bits. 
 *
 * But notice we still encode from the begnning to the end of the file but the last byte will 
 * be stored back so to make it before all other encodings.
 */
template<typename _KeyType>
class HuffEncoder : public Encoder<_KeyType> {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
public:
  //----the specific encoder like HuffEncoder or CanonicalEncoder will decide the outfile name and open it
  HuffEncoder(const std::string& infile_name, std::string& outfile_name) 
      : Encoder<_KeyType>(infile_name, outfile_name) {
    std::string postfix = ".crs";                                 
    if (typeid(type_catergory) == typeid(string_tag))
      postfix += "w";
    if (outfile_name.empty())
      outfile_name = infile_name + postfix;
    this->outfile_ = fopen(outfile_name.c_str(), "wb");
  }
  ~HuffEncoder() {
    delete phuff_tree_;
  }

  ///得到所有符号/单词的编码 
  void gen_encode() {
    //------------------------now the frequency_map_ is ready we can create the HuffTree
    phuff_tree_ = new HuffTree<_KeyType>(this->encode_map_, this->frequency_map_);   
    phuff_tree_->gen_encode(); 
   
    //------------------------记录压缩过程的信息,print_encode并且将建立的二叉树打印到文件中
    std::string out_file_name = this->infile_name_ + "_huff_encode_long.txt";
    std::ofstream out_file(out_file_name.c_str());
    std::cout << "Writting the encode info to the file " << out_file_name << std::endl;
    this->print_encode(out_file); 
    phuff_tree_->print();
  }
 
  ///写入压缩文件头部的huff_tree信息,以便解压缩的时候恢复
  virtual void write_encode_info() {
    phuff_tree_->serialize_tree(this->outfile_);
  }
private:
  HuffTree<_KeyType>* phuff_tree_;  //HuffEnocder use a HuffTree to help gen encode
};

/*
 * 
 */
template<typename _KeyType>
class CanonicalEncoder : public Encoder<_KeyType> {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
public:  
  void gen_encode() {

  }

  void write_encode_info() {

  }
};



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
  
  virtual void get_encode_info() {}
  virtual void decode_file() {}
protected:
  FILE*   infile_;
  FILE*   outfile_;
};

template<typename _KeyType>
class CanonicalDecoder : public Decoder<_KeyType> {
public:

};

template<typename _KeyType>
class HuffDecoder : public Decoder<_KeyType> {
public:
  typedef HuffTree<_KeyType, decode_hufftree>   Tree;
public:
  HuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<_KeyType>(infile_name, outfile_name){
    phuff_tree_ = new Tree(this->infile_, this->outfile_);
  }
  ~HuffDecoder() {
    delete phuff_tree_;
  }
  void get_encode_info() {
    phuff_tree_->build_tree();
  }
  void decode_file() {
    phuff_tree_->decode_file();
  }
private:
  Tree*   phuff_tree_;  //using pointer because we want to later instance of HuffTree
};
//------------------------------------------------------------------------------------------

template<
  template<typename> class _Encoder = HuffEncoder,
  typename _KeyType = unsigned char
  >
class Compressor {
public:
  Compressor(const std::string& infile_name, std::string& outfile_name) 
      : encoder_(infile_name, outfile_name) {}
  /**The overall process of compressing,compressing framework,template pattern*/
  void compress() {
    encoder_.caculate_frequency();
    encoder_.gen_encode();
    //-------------------------------write the compressed file
    encoder_.write_encode_info();
    encoder_.encode_file();
  }
private:
  _Encoder<_KeyType> encoder_;  //using enocder_ right now can be HuffEncoder or CanonicalEncoder 
};

template<
  template<typename> class _Decoder = HuffDecoder,
  typename _KeyType = unsigned char
  >
class Decompressor {
public:
  Decompressor(const std::string& infile_name, std::string& outfile_name) 
      : decoder_(infile_name, outfile_name) {}
  /**The overall process of decompressing, decompressing framework,template pattern*/
  void decompress() {
    //-----------------------------read header--------
    decoder_.get_encode_info();
    //-----------------------------read file content---
    decoder_.decode_file();
  }
private:
  _Decoder<_KeyType> decoder_;   
};

//------------------------------------------------------------------------------------------

} //end of namespace glzip
#ifndef COMPRESSOR_CC_
#include "compressor.cc"
#endif

#endif
