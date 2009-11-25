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
 *  Since it for char type it is not use too much mem,only at most CharSymbolNum characters
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
//but since CharSymbolNum is small it's ok if big num than we shoud free the mem
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
    do_gen_encode(length_);
  }

  /**
   * input: 
   * length array container, and size
   * l is the lenth array, and the length array size n
   * ie. gen_encode(length_, CharSymbolNum)
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
  void do_gen_encode(Container &l)
  {
    
    //note length array input from first --- last , 0--n-1
    //for array of size max_length + 1 ,will index from 1
    //caculate max_len_
    max_len_ = *std::max_element(&l[0], &l[CharSymbolNum]);

    int array_len = max_len_ + 1;  //+1!!
    unsigned int num[array_len];  //0 is not used,num[1] hold in length array the length 1 num 
    unsigned int start_pos_copy[array_len];
    unsigned int next_code[array_len];  //for set encode

    //-----------------init  o(max_length)
    for (int i = 0; i <= max_len_; i++) {
      num[i] = 0;
      start_pos_[i] = 0;
    }
    
    //------------------caclc each length num  o(n) n 
    for (int i = 0; i < CharSymbolNum; i++) {  //range n length of the length array
      num[l[i]] += 1; 
      symbol_[i] = -1;
    }
    num[0] = 0;  //for num[0] may be added since l[i] can be 0
    
    //--------------caculate min_len_
    for (int i = 1; i <= max_len_; i++) {
      if (num[i] != 0) {
        min_len_ = i;
        break;
      }
    }
#ifdef DEBUG
    std::cout << "encoding min length is " << min_len_ << std::endl;
#endif
    
    //--------------caclc the start postion for each length  o(max_length)
    for (int i = 1; i <= max_len_; i++) 
      start_pos_[i] = num[i - 1] + start_pos_[i - 1];
    
    //------------------calc first code for each length  o(max_length)
    //next_code now is a copy of first_code
    first_code_[max_len_] = 0;
    next_code[max_len_] = 0;
    for (int i = max_len_ - 1; i >= 1; i--) {
      first_code_[i] = (first_code_[i + 1] + num[i + 1]) / 2;
      next_code[i] = first_code_[i];
    }

    //to help decdoe easier if we do not store
    //min_len to file tha we can also get min_len 
    //in decoder
    for (int i = 1; i < min_len_; i++) {
      first_code_[i] = 1024;  //max so v < first_code when decode
    }
    

    //-------------------write the encode info since 

    //------------------finish encoding for each symbol and calc symbol array
    //TODO can not be reused for word based hash??
    //or can use symbol_[start_pos[len] + next_code[len] - first_code[len]]
    //so not modify start_pos array and do not need start_pos_copy[],
    //but since n is big we use start_pos[len]++

    //save
    std::copy(start_pos_, start_pos_ + array_len, start_pos_copy);
   
    for (int i = 0; i < CharSymbolNum; i++) {
      int len = l[i];
      if (len) {  //the caracter really exists for cha 256 l[i] > 0
        codeword_[i] = next_code[len]++;  
        symbol_[start_pos_copy[len]++] = i;   //bucket sorting --using index
      }
    }

#ifdef DEBUG
    std::string outfile_name = this->infile_name_ + "_canonical_encode.log";
    std::cout << "Writting canonical encode info to " << outfile_name << std::endl;
    std::ofstream out_file(outfile_name.c_str());
    print_encode(symbol_, CharSymbolNum, out_file);
#endif 
  }

  void print_encode(unsigned int symbol[], int n, std::ostream& out = std::cout)  
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
  //change in 09.11.23 
  //write byte of headermodfied to write int 
  //also using end of encoing mark 
  //These will also be helpful when we want to using word based method!
  //FIXME fix the problem if the encoding length exceeds 32!TODO
  //will not compress well? since header is big?
  void write_encode_info() {
    //------------------------------------------------OK now can write the header out!
    //we need to write the 
    //start_pos  (max_length) ,  we use start_pos_copy which is correct unmodified
    //first_code (max_length) 
    //symbol_    (n)
    fseek (this->outfile_ , 0 , SEEK_SET ); 
    Buffer writer(this->outfile_);
#ifdef DEBUG
    std::cout << "write symbol num is " << CharSymbolNum << std::endl;
#endif
    writer.write_int(CharSymbolNum);
    //FIXME
    for (int i = 0; i< CharSymbolNum; i++) 
      writer.write_int(symbol_[i]);

    //write from index 1
#ifdef DEBUG
    std::cout << "write max encoding length is " << max_len_ << std::endl;
#endif
    writer.write_int(min_len_);
    writer.write_int(max_len_); 
    for (int i = 1; i <= max_len_; i++) { //notice start from 1 fixed!!!!
      writer.write_int(start_pos_[i]);
      writer.write_int(first_code_[i]);
    }

    //encode_file2(writer);
    writer.flush_buf();
    fflush(this->outfile_);     //force to the disk! important!
  }


//modify to add the last end of encoding mark
//instead of storing last byte and left bits
  void encode_file()
  {
    //Cur of infile_ to the start,we must read it again
    fseek (this->infile_ , 0 , SEEK_SET );  
    Buffer reader(this->infile_); 

    Buffer writer(this->outfile_);
    
    encode_each_byte(reader, writer);
    //write end of encode mark
    writer.write_bits(codeword_[CharSymbolNum - 1], length_[CharSymbolNum - 1]);
    //important
    writer.flush_bits();

#ifdef DEBUG
    std::cout << "left bits for last byte is " << writer.left_bits() << std::endl;
#endif

    writer.flush_buf();   //important! need to write all the things int the buf out even buf is not full
    fflush(this->outfile_);     //force to the disk
  }

private:
  
  //this is used during Encoder::encode_file()
  virtual void encode_each_byte(Buffer &reader, Buffer &writer) 
  {
    unsigned char key;
    while(reader.read_byte(key)) {
      writer.write_bits(codeword_[key], length_[key]);
    }
  }

  /**cacluate each symbol's enoding length,will be the same as normal huffman*/
  void get_encoding_length();

private: 
  unsigned int length_[CharSymbolNum];  //store encode length lenghth_ do not support .size() :(
  
  unsigned int codeword_[CharSymbolNum];  //do not use encode_map_  in the base TODO TODO unsinged int?
  
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
template<typename _KeyType>
class CanonicalHuffDecoder : public Decoder<_KeyType> {
public:
  CanonicalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<_KeyType>(infile_name, outfile_name), reader_(this->infile_), writer_(this->outfile_){}

  //read header init all symbol_[] start_pos_[] first_code_[]
  void get_encode_info() 
  {
    //TODO to be modified as reading 4 bytes int not one byte!
    //FIXME
    unsigned int symbol_num;                //symbol num
    reader_.read_int(symbol_num);
    //FIXME
#ifdef DEBUG
    std::cout << "symbol num is " << (int)symbol_num << std::endl;
#endif
       
    //TODO symbol array for word based method must use at least int type
    //FIXME
    for (int i = 0; i < symbol_num; i++) 
      reader_.read_int(symbol_[i]);
    
    //do not use vector but array
    reader_.read_int(min_len_);   //min encoding length
    reader_.read_int(max_len_);   //max encoding length

#ifdef DEBUG
    std::cout << "reading min encoding length is " << min_len_ << std::endl;
    std::cout << "reading max encoding length is " << max_len_ << std::endl;
#endif

    for (int i = 1; i <= max_len_; i++) {  //from 1!
      reader_.read_int(start_pos_[i]);
      reader_.read_int(first_code_[i]);
    } 
  }
  

  //will use reader_ and writer_
  void decode_file() {
#ifdef DEBUG3
    std::cout << "decode in canonical" << std::endl;
    for (int i = reader_.cur_; i < reader_.cur_ + 8; i++) {
      std::cout << std::bitset<8>((unsigned int)(reader_.buf_[i]));
    }
    std::cout << std::endl;
#endif

    //--------------------------------------decode each byte
    unsigned char c;
    int v = 0;        //like a global status for value right now
    int len = 0;      //like a global status for length
    int symbol;
    const int end_mark = CharSymbolNum - 1;
    
    while(1) { 
      reader_.fast_read_byte(c);
      std::bitset<8> bits(c);
      for (int i = 7; i >= 0; i--) {
        v = (v << 1) | bits[i];
        len++;                //length add 1
        if (v >= first_code_[len]) {  //OK in length len we translate one symbol
          symbol = symbol_[start_pos_[len] + v - first_code_[len]];
          
          if (symbol == end_mark) {  //end of file mark!
#ifdef DEBUG
            std::cout << "meetting the coding end mark\n";
#endif          
            writer_.flush_buf();
            fflush(this->outfile_);
            return;
          }
          
          writer_.write_byte(symbol);
          v = 0;
          len = 0;        //fished one translation set to 0
        }
      }
    }
  }

  //void decode_file() {
  //  unsigned char left_bit, last_byte;
  //  reader_.read_byte(left_bit);
  //  reader_.read_byte(last_byte);
  //  //--------------------------------------decode each byte
  //  unsigned char c;
  //  
  //  int v = 0;        //like a global status for value right now
  //  int len = 0;      //like a global status for length
  //  while(reader_.read_byte(c)) 
  //    decode_byte(c, v, len);
  //  //--------------------------------------deal with the last byte
  //  if (left_bit)
  //    decode_byte(last_byte, v, len, (8 - left_bit));
  //  writer_.flush_buf();
  //  fflush(this->outfile_);
  //}

  //FIXME read_bit slow and has problem at the first byte or 
  //void decode_file() {
  //  unsigned char left_bit, last_byte;
  //  reader_.read_byte(left_bit);
  //  reader_.read_byte(last_byte);
  //  //--------------------------------------decode each byte
  //  int bit;
  //  int v = 0;        //like a global status for value right now
  //  int len = 0;      //like a global status for length
  //  while(reader_.read_bit(bit)) {
  //    v = (v << 1) | bit;  //v = v * 2 + bit
  //    len++;
  //    if (v >= first_code_[len]) {
  //      writer_.write_byte(symbol_[start_pos_[len] + v - first_code_[len]]);
  //      v = 0;
  //      len = 0;
  //    }
  //  }
  //  //--------------------------------------deal with the last byte
  //  if (left_bit) {
  //    for (int i = 7; i >= left_bit; i--) {
  //      v = (v << 1) | bit;
  //      len++;
  //      if (v >= first_code_[len]) {
  //        writer_.write_byte(symbol_[start_pos_[len] + v - first_code_[len]]);
  //        v = 0;
  //        len = 0;
  //      }
  //    }  
  //  }
  //  writer_.flush_buf();
  //  fflush(this->outfile_);
  //}


private:

  //void decode_byte(unsigned char c, int &v, int &len, int bit_num = 8){
  //  std::bitset<8> bits(c);
  //  int end = 7 - bit_num;
  //  for (int i = 7; i > end; i--) {
  //    //v = v * 2 + bits[i];  //TODO * and << which is quicker?
  //    v = (v << 1) | bits[i];
  //    len++;                //length add 1
  //    if (v >= first_code_[len]) {  //OK in length len we translate one symbol
  //      writer_.write_byte(symbol_[start_pos_[len] + v - first_code_[len]]);
  //      v = 0;
  //      len = 0;        //fished one translation set to 0
  //    }
  //  }
  //}

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

template<typename _KeyType>
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

   void decode_file() 
  {
#ifdef DEBUG
    std::cout << "decoding in fast canonical" << std::endl;
#endif

    const int end_mark = CharSymbolNum - 1;
    const unsigned int buf_size = 32;  //can use max_len_ as well! TODO try comprare
 
#ifdef DEBUG
    std::cout << "min encoding length is  " << min_len_ << std::endl;
#endif
    //------------------------------------------------first make first_code_ "left_justified"
    //like 0101 ->        0101----------  32 bits
    for (int i = min_len_; i <= max_len_; i++)  
      first_code_[i] <<= (buf_size - i);

    //assume the encoding length do not exceed 32, we can 
    //use unsigned it ro represent the encoding value
    //-------------------------------------------------key process
    BitBuffer bit_buffer(reader_);

    unsigned int v = bit_buffer.read_bits(buf_size);

    unsigned int len = canonical_help::cfind(first_code_, min_len_, v);  //from length 1, find the first that v >= it

    unsigned int symbol = symbol_[start_pos_[len] + ((v - first_code_[len]) >> (buf_size - len))];

    while(symbol != end_mark) {
      writer_.write_byte(symbol);
      v <<= len;                                         
      v |= bit_buffer.read_bits(len);  //insert the next bs - len bits from file stream
      len = canonical_help::cfind(first_code_, min_len_, v);
      symbol = symbol_[start_pos_[len] + ((v - first_code_[len]) >> (buf_size - len))];
    }

#ifdef DEBUG
    std::cout << "searching times in cfind is " << canonical_help::search_times << std::endl;
#endif
    writer_.flush_buf();
    fflush(this->outfile_);
  }

};

//template<typename _KeyType, int TableLength = 8>  //FIXME using this will conlict with Compressor! :( which require only _KeyType
#define  TableLength  8   //use 8bit table
template<typename _KeyType>
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
  void setup_lookup_table() 
  {
    //----------------------------------1 all zero
    int entry_num = (1 << TableLength);
    for (int i = 0; i < entry_num; i ++) {
      lookup_table_[i] = 0;
    }
    
    //----------------------------------2 find all min length for all symbols
    //i is length, j is code
    //Important  i << (-2) != i >> 2 so always shift positive num! 
    unsigned int code, next_code;
    for (int i = max_len_; i > min_len_; i--) {
      code = first_code_[i];
      next_code = (first_code_[i - 1] << 1); //for caculating how many symbols are of encoding length i
      if (TableLength >= i) {
        for (unsigned int j = code; j < next_code; j++) {
          lookup_table_[(j << (TableLength - i))] = i;
        }
      } else {
        for (unsigned int j = code; j < next_code; j++) {
          lookup_table_[(j >> (i - TableLength))] = i;
        }
      }
    }
    //deal with min_len_, from like 10 -> 11 notice the end must be all 1 ,how to prove?
    for (unsigned int j = first_code_[min_len_]; j < (1 << min_len_); j++)
      lookup_table_[(j << (TableLength - min_len_))] = min_len_;

#ifdef DEBUG3
    for (int i = 0; i < entry_num; i++) {
      if (lookup_table_[i]) {
        std::cout << std::bitset<8>(i) << " " << lookup_table_[i] << std::endl;
      }
    }
#endif

    //----------------------------------3 fill all the possible positions like 'symbol + 010' to form 8
    //while before you only have 'symbol + 000' set to the coding length of symbol here 5
    code = 0;
    for (int i = 0; i < entry_num; i++) {
      if (lookup_table_[i] == 0) {
        lookup_table_[i] = code;
      } else {
        code = lookup_table_[i];
      }
    }
    
  }

   void decode_file() 
  {
    setup_lookup_table();

#ifdef DEBUG
    std::cout << "decoding in canonical using lookuptable" << std::endl;
#endif

    const int end_mark = CharSymbolNum - 1;
    const unsigned int buf_size = 32;  //or max_len_ but since using int can not exceed 32

#ifdef DEBUG
    std::cout << "min encoding length is  " << min_len_ << std::endl;
#endif
    
    //32bit left justified
    for (int i = min_len_; i <= max_len_; i++)  
      first_code_[i] <<= (buf_size - i);

    //assume the encoding length do not exceed 32, we can 
    //use unsigned it ro represent the encoding value
    //-------------------------------------------------key process
    BitBuffer bit_buffer(reader_);

    unsigned int v = bit_buffer.read_bits(buf_size);

    unsigned int len = canonical_help::cfind(first_code_, min_len_, v);  //from length 1, find the first that v >= it

    unsigned int symbol = symbol_[start_pos_[len] + ((v - first_code_[len]) >> (buf_size - len))];

    unsigned int vx;
    while(symbol != end_mark) {
      writer_.write_byte(symbol);
      v <<= len;                                         
      v |= bit_buffer.read_bits(len);       //insert the next len bits from file stream
      vx = (v >> (buf_size - TableLength)); //uper 8 bits of v
      len = lookup_table_[vx];              //first search in the look up table
      if (len > TableLength)                //have to search again from len
        len = canonical_help::cfind(first_code_, len, v);
      symbol = symbol_[start_pos_[len] + ((v - first_code_[len]) >> (buf_size - len))];
    }

#ifdef DEBUG3
    std::cout << "searching times in cfind is " << canonical_help::search_times << std::endl;
#endif
    writer_.flush_buf();
    fflush(this->outfile_);
  }

private:
  unsigned int lookup_table_[1 << TableLength];  //now be 2^8 256
};
//------------------------------------------------------------------------------------------

}  //----end of namespace glzip
#ifndef CANONICAL_HUFF_ENCODER_CC_
#include "canonical_huff_encoder.cc"
#endif

#endif  //----end of CANONICAL_HUFF_ENCODER_H_
