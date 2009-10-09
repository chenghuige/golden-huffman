/** 
 *  \file compressor.h
 *  \author ChengHuige
 *  \date  2009-10-6
 */

#ifndef _COMPRESSOR_H_
#define _COMPRESSOR_H_

#include <stdio.h>
#include <string>
#include <iostream>
#include "buffer.h"
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
template<typename _KeyType>
class Encoder {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory type_catergory;
public:
  Encoder(const std::string& infile_name, std::string& outfile_name) {
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
  
  virtual void gen_encode(){}
  void print_encode() {
    do_print_encode(type_catergory());
  }
  virtual void write_encode_info(){}
  
  void encode_file() {
    do_encode_file(type_catergory());
  }

private:
  void do_print_encode(char_tag) {
    std::cout << "The encoding map is as below\n";
    int sum = 0;
    for (int i = 0; i < 256; i++)
      if (frequency_map_[i]) {
        std::cout << ((unsigned char)(i)) << " " << encode_map_[i] << "\n";
        sum++;
      }
    std::cout << "The total number of characters in the file is " << sum << "\n";
  }
  //------------------------------------------
  void do_init(char_tag) {  //for char we use array for frequence map, and vector<string> for encode map,we have to init them.
    for (int i = 0; i < 256; i++)
      frequency_map_[i] = 0;
    encode_map_.resize(256);  
  }
  void init_help(string_tag){}
  //------------------------------------for key type is unsigned char
  void do_caculate_frequency(char_tag) {
    Buffer reader(infile_);  
    unsigned char key;
    while(reader.read_byte(key)) 
      frequency_map_[key] += 1;
    //std::cout << "Finished caculating frequency\n"; 
  }

  void do_encode_file(char_tag)
  {
    long pos = ftell(outfile_);      //The pos right after the head info of compressing
    Buffer writer(outfile_);
    writer.write_byte((unsigned char)(0));      //at last will store leftbit here
    writer.write_byte((unsigned char)(0));      //at last will store the last byte if leftbit > 0
    
    fseek (infile_ , 0 , SEEK_SET ); //Cur of infile_ to the start,we musht read it again
    Buffer reader(infile_); 
    
    unsigned char key;
    while(reader.read_byte(key)) {
      writer.write_string(encode_map_[key]);  //write encode
      //std::cout << encode_map_[key];
    }
    writer.flush_buf();   //important! need to write all the things int the buf out even buf is not full
    //---deal with the last byte
    int left_bits = writer.left_bits();
    //std::cout << "left bits when encoding is " << left_bits << "\n";
    if (left_bits) {
        //fseek(outfile_, 0, pos);   //fixed bug here,went back to the saved pos;  why this is wrong????TODO
        fseek(outfile_, pos, SEEK_SET);
        writer.write_byte((unsigned char)(left_bits));
        for (int i = 0; i < left_bits; i++)
          writer.write_bit(0);    //fill 0 to finish the byte 
        writer.flush_buf();       //write buf to file
    }
    fflush(outfile_);    //force to the disk
  }
  //--------------------------------------for key type is string 
  void do_caculate_frequency(string_tag) {
  }

  void do_encode_file(string_tag) {

  }
protected:
  FILE*                 infile_;
  FILE*                 outfile_;
  EncodeHashMap         encode_map_;
  FrequencyHashMap      frequency_map_;
};

template<typename _KeyType>
class HuffEncoder : public Encoder<_KeyType> {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
public:
  //----the specific encoder like HuffEncoder or CanonicalEncoder will decide the outfile name and open it
  HuffEncoder(const std::string& infile_name, std::string& outfile_name) 
      : Encoder<_KeyType>(infile_name, outfile_name), 
        huff_tree_(this->encode_map_, this->frequency_map_) {
    std::string postfix = ".crs";                                 
    if (typeid(type_catergory) == typeid(string_tag))
      postfix += "w";
    if (outfile_name.empty())
      outfile_name = infile_name + postfix;
    this->outfile_ = fopen(outfile_name.c_str(), "wb");
  }
  

  void gen_encode() {
    huff_tree_.build_tree();
    huff_tree_.gen_encode(); 
    //this->print_encode(); //debug
  }

  virtual void write_encode_info() {
    huff_tree_.serialize_tree(this->outfile_);
  }
private:
  HuffTree<_KeyType> huff_tree_;  //HuffEnocder use a HuffTree to help gen encode
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
  /**The overall process of compressing,compressing framework*/
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
  /**The overall process of decompressing, decompressing framework*/
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

#endif
