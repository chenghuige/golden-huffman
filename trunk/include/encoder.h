/** 
 *  ==============================================================================
 * 
 *          \file   encoder.h
 *
 *        \author   pku_goldenlock@qq.com
 *                  ChengHuige
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
#include <typeinfo>   //type id
#include <functional>
#include <numeric>
#include <iomanip>   //for setw format output


#include "buffer.h"      //fast file read write with buffer
#include "type_traits.h" //char_tag  TypeTraits
#include "tokenizer.h"

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
const int buf_size = 64 * 1024;

template<typename _KeyType>
class Encoder {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
  typedef typename TypeTraits<_KeyType>::HashMap                 HashMap;

public:
  //As a base Encoder only deal with infile_name
  //outfile_name is deal with specific encoder
  //FIXME is it ok to open file in the constutor?? what if we failed to open?
  //FIXME FIXME learn more about expection and safety
  Encoder(const std::string& infile_name)  
    : infile_name_(infile_name), infile_(fopen(infile_name.c_str(), "rb")){
    //infile_ = fopen(infile_name.c_str(), "rb");
    init();
  }

  Encoder(): infile_(NULL), outfile_(NULL){}
 
  //if not given file name at first be sure to set_file before compressing
  void set_infile(const std::string& infile_name) {
    clear();
    infile_name_ = infile_name;
    infile_ = fopen(infile_name.c_str(), "rb");
    init();
  }
  
  void clear() {
    if (infile_) 
      fclose(infile_);
    if (outfile_)
      fclose(outfile_);
    infile_ = NULL;
    outfile_ = NULL;
  }
  
  virtual ~Encoder() {
    //std::cout << "destuct encoder\n";
    clear();
  }

  void caculate_frequency() {
#ifdef DEBUG
    std::cout << "Encoder, !calculate each symbol frequency" 
              << std::endl;
#endif
    do_caculate_frequency(type_catergory());
  }
  
  virtual void gen_encode() = 0;  //must be implemented by specific encoder

  virtual void write_encode_info() = 0;
  
  //TODO add one print encode interface for Encoder
  //may be for the Compressor?
  //FIXME right now print encode all define sperately
  //in spcific encoder

private:
  
  //!! do not name init() and init(char_tag)
  void init() {
    do_init(type_catergory());
  }
  
  void do_init(char_tag) { 
    //for char we use array for frequence map, and vector<string> 
    //for encode map,we have to init them.
    for (int i = 0; i < (CharSymbolNum - 1); i++)
      frequency_map_[i] = 0;
    frequency_map_[CharSymbolNum - 1] = 1;  //for the end of encoing map always be 1
  }
  
  //for key type is unsigned char
  //Notice using the Buffer I have wrote as I used here before or use
  //std::streambuf or istreambuf_iterator will be of similar speed, and
  //code more elegant especiall if using iterator, more adaptable.
  //But here still use fread to get the best speed especailly for large file.
  void do_caculate_frequency(char_tag) {
    //below is the same as
    //while(reader.read_byte(ke)) 
    //   frequency_map_[key] += 1
    unsigned char buf[buf_size];
    int read_num;
    while(1) {
      read_num = fread(buf, 1, buf_size, infile_);  
      for (int i = 0; i < read_num; i++) {
        frequency_map_[buf[i]] += 1;
      }
      if (read_num < buf_size) //file end meet!
        break;
    }
  }

  //---------------------------------------for word(string) based below
  void do_init(string_tag) {}
  
  template <typename _HashMap,typename _Token = std::string>
  struct CalcFrequency: public std::unary_function<_Token, void> {
    explicit CalcFrequency(_HashMap& frequency_map)
      :f_map_(frequency_map) {}
    
    void operator() (_Token& token) {
      f_map_[token] += 1;
    }
  
  private:
    _HashMap& f_map_;  
  };

  void do_caculate_frequency(string_tag) 
  {
#ifdef DEBUG
    std::cout << "word huffman calc frequency\n";
#endif
    std::ifstream ifs(infile_name_.c_str());
    typedef std::istreambuf_iterator<char>  Iter;
    Iter iter(ifs);
    Iter end;
    
    typedef CalcFrequency<HashMap> Func;
    typedef Tokenizer<Iter,Func > Tokenizer;
    Func word_func(frequency_map_[0]);
    Func non_word_func(frequency_map_[1]);
    
    Tokenizer tokenizer(iter, end, word_func, non_word_func);

    tokenizer.split();

    //wether start from a word or not
    word_first_ = tokenizer.is_word_first();
    
    //for word add eof to mark the end
    //Note we use EOF -1 to mark than end, and use "" to mark if
    //we split 88889999 to 8888+""+9999 this feature is TODO
    std::string eof;
    eof.push_back(-1);
    frequency_map_[0][eof] = 1;
    //for non word also add eof mark
    frequency_map_[1][eof] = 1;

#ifdef DEBUG
    std::cout << "word num is " << frequency_map_[0].size() << std::endl;
    std::cout << "non word num is " << frequency_map_[1].size() << std::endl;
#endif
  }

protected:
  FILE*                 infile_;    
  FILE*                 outfile_;

  FrequencyHashMap      frequency_map_;      //all encoders will use the same frequcy_map_

  bool                  word_first_;        //for huff word only,need to know when calc fre,so must here:(

  std::string           infile_name_;        //for debug gen_enocde print log 

};

//---------------------------------------------------------------------Decoder
/**
 * Decoder will provide the outfile name and outfile open infile and outfile
 * Derived decoder will provide 
 * get_encode_info()
 * decode_file()
 */
template<typename _KeyType>
class Decoder {
public:
  Decoder(const std::string& infile_name, std::string& outfile_name) {
    infile_ = fopen(infile_name.c_str(), "rb");
    if (outfile_name.empty())
      outfile_name = infile_name + ".de";
    outfile_ = fopen(outfile_name.c_str(), "wb");
  }
  virtual ~Decoder() {
    //std::cout << "destruct decoder\n";
    fclose(infile_);  //FIXME double free for huff word decoder?
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
