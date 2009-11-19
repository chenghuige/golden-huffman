/** 
 *  ==============================================================================
 * 
 *          \file   normal_huff_encoder.h 
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-18 16:12:13.356033
 *  
 *   Description:   Class of NormalHuffEncoder and NormalHuffDecoder
 *                  
 *                  Dervie from HuffEndoder and HuffDecoder
 *
 *                  They will use HuffTree to do the coding and decoding.
 *  ==============================================================================
 */


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

#ifndef NORMAL_HUFF_ENCODER_H_
#define NORMAL_HUFF_ENCODER_H_

#include "encoder.h"    //Encoder and Decoder
#include "huff_tree.h"  //huff_tree do the endoing and decoding

namespace glzip {
//--------------------------------------------------------------------------NormalHuffEncoder
/* *
 * Class NormalHuffEnocder  --Encoder using the normal huffman method
 */
template<typename _KeyType>
class NormalHuffEncoder : public Encoder<_KeyType> {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
  typedef EncodeHuffTree<_KeyType>   Tree;
public:
  //----the specific encoder like HuffEncoder or CanonicalEncoder will decide the outfile name and open it
  NormalHuffEncoder(const std::string& infile_name, std::string& outfile_name) 
      : Encoder<_KeyType>(infile_name, outfile_name) {
    std::string postfix = ".crs";                                 
    if (typeid(type_catergory) == typeid(string_tag))
      postfix += "w";
    if (outfile_name.empty())
      outfile_name = infile_name + postfix;
    this->outfile_ = fopen(outfile_name.c_str(), "wb");
  }
  ~NormalHuffEncoder() {
    delete phuff_tree_;
  }

  ///得到所有符号/单词的编码 
  void gen_encode() {
    //------------------------now the frequency_map_ is ready we can create the HuffTree
    phuff_tree_ = new Tree(this->encode_map_, this->frequency_map_);   
    phuff_tree_->gen_encode(); 
    
    #ifdef DEBUG2  //DEBUG2 means dumping log while DEBUG means using gtest checking 
      print_log();
    #endif
  }

  // std::string tree_file_name  should be *.dot
  void print_log(std::string out_file_name = "",  std::string tree_file_name = "") {
    //------------------------记录压缩过程的信息,print_encode并且将建立的二叉树打印到文件中
    if (out_file_name == "")
      out_file_name = this->infile_name_ + "normal_huff_encode_long.txt";
    std::ofstream out_file(out_file_name.c_str());
    std::cout << "Writting the encode info to the file " << out_file_name << std::endl;
    this->print_encode(out_file); 
    out_file.close();
    if (tree_file_name == "")
      phuff_tree_->print();
    else
      phuff_tree_->print(tree_file_name);
  }
 
  ///写入压缩文件头部的huff_tree信息,以便解压缩的时候恢复
  virtual void write_encode_info() {
    phuff_tree_->serialize_tree(this->outfile_);
  }
private:
  Tree* phuff_tree_;  //HuffEnocder use a HuffTree to help gen encode
};

//--------------------------------------------------------------------------NormalHuffDecoder
/* *
 * Class NormalHuffDeocder  --Decoder using the normal huffman method
 */
template<typename _KeyType>
class NormalHuffDecoder : public Decoder<_KeyType> {
public:
  typedef DecodeHuffTree<_KeyType>   Tree;
public:
  NormalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<_KeyType>(infile_name, outfile_name){
    phuff_tree_ = new Tree(this->infile_, this->outfile_);
  }
  ~NormalHuffDecoder() {
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

}       //----end of namesapce glzip

#ifndef NORMAL_HUFF_ENCODER_CC_
#include "normal_huff_encoder.cc"
#endif

#endif  //----end of NORMAL_HUFF_ENCODER_H_

