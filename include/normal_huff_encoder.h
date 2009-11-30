/** 
 *  ==============================================================================
 * 
 *          \file   normal_huff_encoder.h 
 *
 *        \author   pku_goldenlock@qq.com
 *                  ChengHuige
 *
 *          \date   2009-11-18 16:12:13.356033
 *  
 *   Description:   Class of NormalHuffEncoder and NormalHuffDecoder
 *                  
 *                  Dervie from HuffEndoder and HuffDecoder
 *
 *                  They will use HuffTree to do the coding and decoding.
 *
 *                  Notice normal_huff_encoder is just for experiment puporse
 *                  it will only support char based method!
 *                  Since work now it will not be supported and modified any more!
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
//TODO may be can use composite instead of deriving
//NormalHuffEncoder using Encoder.
template<typename _KeyType = unsigned char>
class NormalHuffEncoder : public Encoder<_KeyType> {
public:
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap        FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap           EncodeHashMap;
  typedef typename TypeTraits<_KeyType>::type_catergory          type_catergory;
  typedef EncodeHuffTree<_KeyType>   Tree;
  typedef Encoder<_KeyType>          Base;
  using Base::infile_;
  using Base::outfile_;
public:
  //----the specific encoder like HuffEncoder or CanonicalEncoder will decide the outfile name and open it
  //TODO right now infile_name not const!
  NormalHuffEncoder(const std::string& infile_name, std::string& outfile_name) 
      : Encoder<_KeyType>(infile_name) {
    set_out_file(infile_name, outfile_name);
    init_nhuff();  //important!!! do not use init() as in the base!!!
  }

  NormalHuffEncoder(): phuff_tree_(NULL) {}

  //TODO need to be virtual? Now is Ok?
  void set_out_file(const std::string& infile_name, std::string& outfile_name) {
    std::string postfix = ".crs";                                 
    if (typeid(type_catergory) == typeid(string_tag))
      postfix += "w";
    if (outfile_name.empty())
      outfile_name = infile_name + postfix;
    this->outfile_ = fopen(outfile_name.c_str(), "wb");
  }

  void set_file(const std::string& infile_name, std::string& outfile_name) {
    clear_tree();       //we need to re start
    init_nhuff();
    
    this->set_infile(infile_name);    //base calss will deal with infile
    set_out_file(infile_name, outfile_name);
  }

  ~NormalHuffEncoder() {
    clear_tree();
  }

  void clear_tree() {
    if (phuff_tree_)
      delete phuff_tree_;
  }
  
 
  ///得到所有符号/单词的编码 
  void gen_encode() {
#ifdef DEBUG
    std::cout << "NormalHuffEncoder, gen encode" << std::endl;
#endif
    //------------------------now the frequency_map_ is ready we can create the HuffTree
    phuff_tree_ = new Tree(encode_map_, this->frequency_map_);   
    phuff_tree_->gen_encode(); 
#ifdef DEBUG   
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
    print_encode(out_file); 
    out_file.close();
#ifdef DEBUG2               //only draw graph when DEBUG2 is setting
    if (tree_file_name == "")
      phuff_tree_->print();
    else
      phuff_tree_->print(tree_file_name);
#endif 
  }
 
  ///写入压缩文件头部的huff_tree信息,以便解压缩的时候恢复
  virtual void write_encode_info() {
    phuff_tree_->serialize_tree(this->outfile_);
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

  void encode_file()
  {
    long long pos = ftell(outfile_);      //The pos right after the head info of compressinon
    Buffer writer(outfile_);
    writer.write_byte((unsigned char)(0));      //at last will store leftbit here
    writer.write_byte((unsigned char)(0));      //at last will store the last byte if leftbit > 0
  
    fseek (infile_ , 0 , SEEK_SET ); //Cur of infile_ to the start,we must read it again
    Buffer reader(infile_); 
  
    //TODO one poosible way is to let the while be virtual so 
    //we will not call so many virtual functions in while
    //different encoder will use differnt method
    encode_each_byte(reader, writer);
 
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

private:
  void init_nhuff() {
    do_init_nhuff(type_catergory());
  }

  void do_init_nhuff(char_tag) {
    //FIXME carefully deal and remove all 256
    encode_map_.resize(256);  
  }

  void encode_each_byte(Buffer &reader, Buffer &writer) { 
    unsigned char key;
    while(reader.read_byte(key)) 
      writer.write_string(this->encode_map_[key]);
  }

  void do_print_encode(char_tag, std::ostream& out)
  {
    using namespace std;

    out << "The input file is " <<  this->infile_name_ << "\n";
    //-----统计文件中一共有多少个byte文件大小
    long long start = 0;
    long long byte_sum =std::accumulate(this->frequency_map_, this->frequency_map_ + 256, start);
    out << "The total bytes num is " << byte_sum << "\n";
    out << "\n";
    //-----统计文件中一共出现了多少个不同的字符byte
    //-----统计平均码长,忽略头部信息的压缩率 
    //TODO gcc4.5 支持lamada 否则用foreach之类去外面定义仿函数和函数得不偿失
    //boost lamada?
    //TODO c++0x Stream Fusion 避免多次循环? 现在提出来多一个循环,影响效率
    //但是代码更清楚,一个循环一个单独的功能,主要还是能够为了先显示字符数目
    //long long character_sum = std::count_if
    long long character_sum = 0;
    float avg_length = 0;  //in average how many bits are needed for one character after encode ,before encode is 8
    for (int i = 0; i < 256; i++) {
      if (this->frequency_map_[i]) {
        character_sum++;
        avg_length += encode_map_[i].length() * this->frequency_map_[i];
      }
    }
    avg_length /= float(byte_sum);
  
    out << "The total number of different characters in the file is " 
        << character_sum << "\n";
    out << "The average encoding length per character is " << avg_length << "\n";
    out << "So not consider the header the approximate compressing ration should be " 
        << avg_length/8.0 << "\n"; 
    out << "\n";
    //-----打印编码信息
    out << "The encoding map:\n\n";
    
    out << setiosflags(ios::left) << setw(20) << "Character"
                                  << setw(20) << "Times"
                                  << setw(20) << "Frequence" 
                                  << setw(20) << "EncodeLength"
                                  << setw(30) << "Encode" << "\n\n";
    for (int i = 0; i < 256; i++) {
      if (this->frequency_map_[i]) {
        //左对齐,占位
        unsigned char key = i;
        if (key == '\n') {
         out << setiosflags(ios::left) << setw(20) << "\\n"  
                                << setw(20) << this->frequency_map_[i]
                                << setw(20) << setprecision(3) << this->frequency_map_[i]/(float)byte_sum
                                << setw(20) << encode_map_[i].length()
                                << setw(30) << encode_map_[i] << "\n";
        } else if (key == ' ') {
          out << setiosflags(ios::left) << setw(20) << "space" 
                                << setw(20) << this->frequency_map_[i]
                                << setw(20) << setprecision(3) << this->frequency_map_[i]/(float)byte_sum
                                << setw(20) << encode_map_[i].length()
                                << setw(30) << encode_map_[i] << "\n";
        } else {
          out << setiosflags(ios::left) << setw(20) << key
                                << setw(20) << this->frequency_map_[i]
                                << setw(20) << setprecision(3) << this->frequency_map_[i]/(float)byte_sum
                                << setw(20) << encode_map_[i].length()
                                << setw(30) << encode_map_[i] << "\n";
        }
      }
    }
    //out.close() //not close for std::ostream struct std::basic_ostream<char, std::char_traits<char> >
  }

private:
  EncodeHashMap         encode_map_;    //normal huffman use encode_map_ which is char->string while canonical huff not use 
                                        //this for store encode it use a int array codeword_ and int array lenth_
  Tree* phuff_tree_;  //HuffEnocder use a HuffTree to help gen encode
};

//--------------------------------------------------------------------------NormalHuffDecoder
/* *
 * Class NormalHuffDeocder  --Decoder using the normal huffman method
 */
template<typename _KeyType = unsigned char>
class NormalHuffDecoder : public Decoder<_KeyType> {
public:
  typedef DecodeHuffTree<_KeyType>   Tree;
public:
  NormalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<_KeyType>(infile_name, outfile_name), phuff_tree_(NULL){
    //phuff_tree_ = new Tree(this->infile_, this->outfile_);
  }
  ~NormalHuffDecoder() {
    clear_tree();
  }
  void clear_tree() {
    if (phuff_tree_) {
      delete phuff_tree_;
    }
  }
  void get_encode_info() {
    phuff_tree_ = new Tree(this->infile_, this->outfile_);
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

