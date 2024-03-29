/** 
 *  ==============================================================================
 * 
 *          \file   compressor.h
 *
 *        \author   pku_goldenlock@qq.com
 *                  ChengHuige
 *
 *          \date   2009-11-18 16:12:13.356033
 *  
 *   Description:   The overall frame work of Compressor and Decompressor
 *                  Template design pattern.
 *
 *                  Compressor will use specific encoder while 
 *                  Decompressor use specific decoder to finsh work.
 *                
 *                  They will provide the overall process of compressing 
 *                  and decompressing.
 *  ==============================================================================
 */

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


#ifndef COMPRESSOR_H_
#define COMPRESSOR_H_

#include <string>
#include <fstream>

namespace glzip{

//---------------------------------------------------------Compressor
template<typename _Encoder>
class Compressor {
public:
  Compressor(const std::string& infile_name, std::string& outfile_name) 
      : encoder_(infile_name, outfile_name) {}
  
  Compressor() {}
  
  //TODO add set_file for decompressor
  void set_file(const std::string& infile_name, std::string& outfile_name) {
    encoder_.set_file(infile_name, outfile_name);
  }

  void clear() {
    encoder_.clear();
  }

  /**The overall process of compressing,compressing framework,template pattern*/
  void compress() {
    encoder_.caculate_frequency();   //read file and calc           --done by Encoder
    encoder_.gen_encode();           //gen encode based on frequnce --done by specific encoder
    //-------------------------------write the compressed file
    //notice some encoder might write encode info just at the last step of gen_encode so this will
    //not use a sperate process, choices 1. pass paremeter 2. as class varaibe sharing
    //TODO which is better?
    /*write header*/
    encoder_.write_encode_info();    //write outfile header(encoding info) ---done by specific encoder
    /*encode and write whole file */
    encoder_.encode_file();          //read infile,translate to outfile,   ---done by specific encoder now 
  }                                  //may be normal huff and canoncial should share but sitll I will not 
                                     //support normal huff any more just use old version
private:
  _Encoder encoder_;  //using enocder_ right now can be HuffEncoder or CanonicalEncoder 
};

//---------------------------------------------------------Decompressor
//TODO using  template<typename> class _Decoder = NormalHuffDecoder will limit the _Decoder type
template<typename _Decoder>
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
  _Decoder decoder_;   
};

//------------------------------------------------------------------------------------------

} //end of namespace glzip
#ifndef COMPRESSOR_CC_
#include "compressor.cc"
#endif

#endif  //end of  COMPRESSOR_H_

