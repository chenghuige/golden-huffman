/** 
 *  ==============================================================================
 * 
 *          \file   canonical_huffword.h
 *
 *        \author   chenghuige@gmail.com
 *          
 *          \date   2009-11-28 19:43:46.219853
 *  
 *   Description:   Implementation of canonnical huff word based
 *                  encoder
 *  ==============================================================================
 */

#ifndef CANONICAL_HUFFWORD_H_
#define CANONICAL_HUFFWORD_H_

#include "encoder.h"
#include <string>
#include <queue>
#include <deque>
#include <vector>
#include <functional>

//for serialization
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#ifdef DEBUG
#include <gtest/gtest.h>
#endif
#include <bitset>

#include <heap_func.h> //for shift down


namespace glzip {

//forwod declare
template<typename _KeyType>
class CanonicalHuffEncoder;

//specialized word based canonical huff encoder
template<>
class CanonicalHuffEncoder<std::string> : public Encoder<std::string> {
private:
  //---------------------prepare for the priority queue
  typedef TypeTraits<std::string>::FrequencyHashMap        FrequencyHashMap;
  typedef TypeTraits<std::string>::type_catergory          type_catergory;
  typedef TypeTraits<std::string>::HashMap                 HashMap;

  typedef Encoder<std::string>        Base;
  using Base::infile_;
  using Base::outfile_;
  using Base::frequency_map_;
private:
  std::string outfile_name_;          //save for using fstream 
public:  
  CanonicalHuffEncoder(const std::string& infile_name, std::string& outfile_name) 
      : Encoder<std::string>(infile_name) {
    set_out_file(infile_name, outfile_name);
  }

  CanonicalHuffEncoder() {}

  void set_out_file(const std::string& infile_name, std::string& outfile_name)
  {
    std::string postfix(".crs3");
    if (outfile_name.empty())
      outfile_name = infile_name + postfix;
    outfile_name_ = outfile_name;
  }

  void set_file(const std::string& infile_name, std::string& outfile_name)
  {
    this->set_infile(infile_name);
    set_out_file(infile_name, outfile_name);
  }
  
  void gen_encode()
  {
#ifdef DEBUG
    std::ofstream ofs("freq_word.log");
    print_frequency(frequency_map_[0], ofs);
    ofs.clear();
#endif
    //------------------------------gen encode length for word hash map
    int max_word_encoding_length 
      = get_encoding_length(frequency_map_[0]);
#ifdef DEBUG
    std::ofstream ofs2("length_word.log");
    print_length(frequency_map_[0], ofs2);
    ofs2.close();
#endif

#ifdef DEBUG
    std::ofstream ofs3("freq_nonword.log");
    print_frequency(frequency_map_[1], ofs3);
    ofs3.clear();
#endif
    //------------------------------gen encode length for non word hash map
    int max_nonword_encoding_length
      = get_encoding_length(frequency_map_[1]);
#ifdef DEBUG
    std::ofstream ofs4("length_nonword.log") ;
    print_length(frequency_map_[1], ofs4);
    ofs4.close();
#endif

   std::ofstream fout(outfile_name_.c_str(), std::ios::binary);//把对象写到file.txt文件中
   boost::archive::binary_oarchive oa(fout);   //文本的输出归档类，使用一个ostream来构造

  //---------------gen encode for word and non word also will write the header info
  //at the end, notice will first gen encode and write info for non word if
  //word_first_ == false
  if (this->word_first_) {
      //------------------------------really gen encode using length info for word
    do_gen_encode(frequency_map_[0], max_word_encoding_length, oa);
#ifdef DEBUG
    std::string outfile_name = this->infile_name_ + "_word_canonical_encode.log";
    std::cout << "Writting canonical encode info to " << outfile_name << std::endl;
    std::ofstream out_file(outfile_name.c_str());
    print_encode(frequency_map_[0], out_file);
#endif 
    //------------------------------really gen encode using length info for non word
    do_gen_encode(frequency_map_[1], max_nonword_encoding_length, oa);
#ifdef DEBUG
    std::string outfile_name2 = this->infile_name_ + "_nonword_canonical_encode.log";
    std::cout << "Writting canonical encode info to " << outfile_name2 << std::endl;
    std::ofstream out_file2(outfile_name2.c_str());
    print_encode(frequency_map_[1], out_file2);
#endif 
  }
  else {
    //------------------------------really gen encode using length info for non word
    do_gen_encode(frequency_map_[1], max_nonword_encoding_length, oa);
#ifdef DEBUG
    std::string outfile_name2 = this->infile_name_ + "_nonword_canonical_encode.log";
    std::cout << "Writting canonical encode info to " << outfile_name2 << std::endl;
    std::ofstream out_file2(outfile_name2.c_str());
    print_encode(frequency_map_[1], out_file2);
#endif 
    //------------------------------really gen encode using length info for word
    do_gen_encode(frequency_map_[0], max_word_encoding_length, oa);
#ifdef DEBUG
    std::string outfile_name = this->infile_name_ + "_word_canonical_encode.log";
    std::cout << "Writting canonical encode info to " << outfile_name << std::endl;
    std::ofstream out_file(outfile_name.c_str());
    print_encode(frequency_map_[0], out_file);
#endif 
   }

  //finished write header close
  fout.close();
}

  
  //gen encode for hmp, and will calc min_len also
  void do_gen_encode(HashMap& hmp, int max_len,
               boost::archive::binary_oarchive& oa)
  {
    const unsigned int SymbolNum = hmp.size();
    typedef HashMap::iterator Iter;
    Iter iter = hmp.begin();
    Iter end = hmp.end();

    int array_len = max_len + 1;  //+1!!
    unsigned int num[array_len];  //0 is not used,num[1] hold in length array the length 1 num 
    unsigned int start_pos_copy[array_len];
    unsigned int next_code[array_len];  //for set encode
    /** the sorted array of symbol index like length_[symbol[0]] is min length.*/
    unsigned int symbol_index[SymbolNum]; 
    /** like 0000 for max_encoding_length the min encoing value of each encoding length */
    unsigned int first_code[array_len];      //max_encoding_length should <= 32, we need at most 32+1, now use 64
  
    /** the index position of the min encoing in symbol for each encoding length */
    unsigned int start_pos[array_len];       

    //-----------------init  o(max_length)
    for (int i = 0; i <= max_len; i++) {
      num[i] = 0;
      start_pos[i] = 0;
    }
    
    //------------------caclc each length num  o(n) n 
    //differnt writting style with char based since depending on hash map
    for (unsigned int i = 0; i < SymbolNum; i++, ++iter) {  //range n length of the length array
      num[(*iter).second.fl.length] += 1;  
      symbol_index[i] = -1;
    }
    num[0] = 0;  //for num[0] may be added since l[i] can be 0
    
    //--------------caculate min_len
    int min_len;
    for (int i = 1; i <= max_len; i++) {
      if (num[i] != 0) {
        min_len = i;
        break;
      }
    }
#ifdef DEBUG
    std::cout << "encoding min length is " << min_len << std::endl;
#endif
    
    //--------------caclc the start postion for each length  o(max_length)
    for (int i = 1; i <= max_len; i++) 
      start_pos[i] = num[i - 1] + start_pos[i - 1];
#ifdef DEBUG2
    std::cout << "Show the num and start_pos array" << std::endl;
    for (int i = 0; i <= max_len; i++) {
      std::cout << num[i] << " " << start_pos[i] << " " << i <<std::endl;
    }
#endif
    //------------------calc first code for each length  o(max_length)
    //next_code now is a copy of first_code
    first_code[max_len] = 0;
    next_code[max_len] = 0;
    for (int i = max_len - 1; i >= 1; i--) {
      first_code[i] = (first_code[i + 1] + num[i + 1]) / 2;
      next_code[i] = first_code[i];
    }
 
    //to help decdoe easier if we do not store
    //min_len to file tha we can also get min_len 
    //in decoder
    for (int i = 1; i < min_len; i++) {
      first_code[i] = 1000000;  //max so v < first_code when decode
    }

    //------------------finish encoding for each symbol and calc symbol array
    std::copy(start_pos, start_pos + array_len, start_pos_copy);

    //here differnt with char based
    iter = hmp.begin();
    for (unsigned int i = 0; i < SymbolNum; i++, ++iter) {
      unsigned int len = (*iter).second.fl.length;
      (*iter).second.codeword = next_code[len]++; 
#ifdef DEBUG2
      EXPECT_LT(start_pos_copy[len], SymbolNum) << i << " " << len;
#endif
      symbol_index[start_pos_copy[len]++] = i;   //bucket sorting --using index
    }
    
    //------------------------------------at the end we write header
    write_header(hmp, max_len, min_len,
                 symbol_index, first_code, start_pos,
                 oa);
  }
 
    void print_encode(HashMap& hmp, std::ostream& out = std::cout)
    {
      using namespace std;
      typedef HashMap::iterator Iter;
      Iter end = hmp.end();
      cout << "printing encode" << endl;
      for (Iter iter = hmp.begin();iter != end; ++iter) {
        out << setiosflags(ios::left) 
            << setw(20) << iter->first 
            << setw(20) << (iter->second).fl.length
            //<< setw(20) << (iter->second).codeword 
            << setw(20) << get_encode_string((iter->second).codeword, (iter->second).fl.length)
            << endl;
      }
    }

    void print_frequency(HashMap& hmp, std::ostream& out = std::cout) 
    {
      using namespace std;
      typedef HashMap::iterator Iter;
      Iter end = hmp.end();
      cout << "printing frequency" << endl;
      for (Iter iter = hmp.begin();iter != end; ++iter) {
        out << setiosflags(ios::left) 
            << setw(20) << iter->first 
            << setw(20) << (iter->second).fl.frequency << endl;
      }
    }
  
  void print_length(HashMap& hmp, std::ostream& out = std::cout) 
  {
    using namespace std;
    typedef HashMap::iterator Iter;
    Iter end = hmp.end();
    cout << "printing encoding length" << endl;
    for (Iter iter = hmp.begin();iter != end; ++iter) {
      out << setiosflags(ios::left) 
          << setw(20) << iter->first 
          << setw(20) << (iter->second).fl.length << endl;
    }
  }

  std::string get_encode_string(unsigned int code, unsigned int len)
  {
    std::string s;
    unsigned int mask = 1 << (len - 1);
    for (unsigned int i = 0; i < len; i++) {
      if ((code & mask) == 0) {
        s.push_back('0');
      } else {
        s.push_back('1');
      }
      mask >>= 1;
    }
    return s;
  }

  /**
   * Will write symbols,max encoding, min encoding, 
   * symbols index, first code and start pos, the sequence is
   * 由于symbol不打算从hash中移出来存到数组里了，而整个hash
   * 只序列化它的key我还不会，所以要存储symbol num用于解信息 TODO
   * 1. symbol num             1 int 
   * 2. max encoding length    1 int
   * 3. min encoding length    1 int
   * 4. symbol       array     symbol num
   * 5. symbol_index array     symbol num 
   * 6. first_code   array     max_len + 1
   * 7. start_pos    array     max_len + 1
   * 另外关于序列话数组似乎这种运行时决定大小的不行。
   * 用的话改vector吧。
   */
  void write_header(HashMap& hmp, int max_len, int min_len, 
                           unsigned int symbol_index[],
                           unsigned int first_code[],
                           unsigned int start_pos[],
                  boost::archive::binary_oarchive& oa)
  {
    const unsigned int SymbolNum = hmp.size();
    typedef HashMap::iterator Iter;
    Iter iter = hmp.begin();
    Iter end = hmp.end();

    oa << SymbolNum << max_len << min_len;
    
    //store symbol string 
    for (; iter != end; ++iter) {
      oa << iter->first;         
    }
   
    //store symbol index
    for (int i = 0; i < SymbolNum; i++) {
      oa << symbol_index[i];
    }
    
    //store first code
    for (int i = 0; i <= max_len; i++) {
      oa << first_code[i];
    }

    //store start pos
    for (int i = 0; i <= max_len; i++) {
      oa << start_pos[i];
    }
  }

  /**
   * This is empty since we write encode 
   * actually when at the end of gen encode (write_header)
   * becasue we want to pass symbol index array 
   * and we want to allocate 
   * symbol index array space only when needed, 
   * so do not as a class data member! using vector as a 
   * data member may be a choice but we want to be as fast
   * as possible
   */
  void write_encode_info() {

  }
 
  template <typename _HashMap,typename _Token = std::string>
  struct WriteEncode: public std::unary_function<_Token, void> {
    explicit WriteEncode(_HashMap& frequency_map, Buffer& writer)
      :map_(frequency_map), writer_(writer) {}
    
    void operator() (_Token& token) {
      writer_.write_bits(map_[token].codeword, map_[token].fl.length); 
    }
  
  private:
    _HashMap& map_;  
    Buffer&   writer_;    //fixed here Buffer type  
  };

  void encode_file()
  {
    //append to the output file! since have written header 
    this->outfile_ = fopen(outfile_name_.c_str(), "ab");
    Buffer writer(this->outfile_);
    
    //read the input file for the second time
    std::ifstream ifs( (this->infile_name_).c_str());
    typedef std::istreambuf_iterator<char>  Iter;
    Iter iter(ifs);
    Iter end;

    typedef WriteEncode<HashMap> Func;
    typedef Tokenizer<Iter,Func > Tokenizer;
    Func word_func(frequency_map_[0], writer);
    Func non_word_func(frequency_map_[1], writer);
    
    
    Tokenizer tokenizer(iter, end, word_func, non_word_func);

    tokenizer.split(); //或者也可以将split函数加上函数对象变量
    ifs.close();

    bool word_last = tokenizer.is_word_last(); 

   //write end of encode mark
    std::string eof;
    eof.push_back(-1);
    //if the last is a word the will mark of non word end
    if (word_last) {  
      writer.write_bits(frequency_map_[1][eof].codeword, 
                        frequency_map_[1][eof].fl.length);
    }
    else {
      writer.write_bits(frequency_map_[0][eof].codeword, 
                        frequency_map_[0][eof].fl.length);
    }
#ifdef DEBUG
    std::cout << "left bits for last byte is " << writer.left_bits() << std::endl;
#endif
    //important make sure the last byte output
    writer.flush_bits();

    writer.flush_buf();   //important! need to write all the things int the buf out even buf is not full
    fflush(this->outfile_);     //force to the disk
  }

private:
  /*
   * Cacluate each symbol's enoding length.
   * The method is similar as in MG book,p47
   * Uisng n=hmp.size() additional space only
   * So the entire sapcing I'm using until now
   * is 
   * 2*hmp.size()*sizeof(int) + 1*hmp.size()*sizeof(int)
   * + string space(for storing names)
   * or hash table size + 1*hmp.size()*sizeof(int)
   *
   * notice both word and non word hmp have to be
   * considered above only consider one hmp.
   *
   * Will also caculating the max encoding length;
   * return max encoding length
   *    
   */
  struct greater_pointer {
    bool operator()(unsigned int __x, unsigned int  __y) const { 
      return *(reinterpret_cast<unsigned int*>(__x)) > *(reinterpret_cast<unsigned int*>(__y)); 
    }
  };

  static int get_encoding_length(HashMap& hmp)
  {
    //we use heap_array first to store pointers point to 
    //hashp map frequency then will conver to use as int
    //so make sure sizeof(int*) == sizeof(int)!  FIXME 64bit will wrong?
    //what if we store index? *(iter + 3) ok? still fast? for hash,learn more about hash
    const unsigned int ksize = hmp.size();
    unsigned int heap_array[ksize]; //The only addition space we need 
    typedef HashMap::iterator Iter;
    Iter iter = hmp.begin();
    Iter end = hmp.end();
    //--------------------------------------------------make a heap
    for (unsigned int i = 0; i < ksize; i++, ++iter) {
      heap_array[i] = reinterpret_cast<unsigned int>(&((*iter).second.fl.frequency));
      std::push_heap(heap_array, heap_array + i + 1, 
          greater_pointer());
    }
    //--------------------------------------------------form a tree, loop n-1 times
    for (unsigned int i = 0; i < (ksize - 1); i++) {
      //first pop two elements, these two will go to 
      //the last 2 pos in the heap_array
      pop_heap(heap_array, heap_array + ksize - i,
          greater_pointer());
      
      unsigned int* m1 = reinterpret_cast<unsigned int*>(heap_array[ksize - i - 1]);
      unsigned int* m2 = reinterpret_cast<unsigned int*>(heap_array[0]);

      //FIXME should not exceed unsigned int range 2^32 that means not more than 4G file
      //right now the smallest is          *heap_array[ksize - i - 1]
      //the second smallest right now is   *heap_array[0]
      //after the add , heap_array[ksize - i - 1] will be like a interanl node in the tree
      heap_array[ksize - i - 1] = *reinterpret_cast<unsigned int*>(heap_array[ksize - i - 1]) + 
                                  *reinterpret_cast<unsigned int*>(heap_array[0]); 

      //the heap will shrank size 1 only each loop
      //will make a new heap [0, ksize - i - 1),
      //with pos 0 store reinterpret_cast<unsigned int>(&(heap_array[ksize - i - 1]))
      //and then shift down to keep it a heap
      glzip::heap_shift_down(heap_array, heap_array + ksize - i - 1, 
                             reinterpret_cast<unsigned int>(&(heap_array[ksize - i - 1])),
                             greater_pointer()
                            );

      *m1 = ksize - i - 1;  //store index will be ok, pointer from 2 son node to parent
      *m2 = ksize - i - 1;
    }
    //--------------------------------------------------calc length,max_len_
    //child  length = it's parent length + 1, o(n) n = ksize
    //deal with ksize -1 interal node
    int max_len = 0;
    heap_array[1] = 0;  //set root to 0
    for (unsigned int i = 2; i < ksize; i++) {
      heap_array[i] = heap_array[heap_array[i]] + 1;
    }
    //deal with ksize leaf
    for (iter = hmp.begin(); iter != end; ++iter) {
      (*iter).second.fl.length = heap_array[(*iter).second.fl.length] + 1;
      
      if ((*iter).second.fl.length > max_len) {
        max_len = (*iter).second.fl.length; 
      }
    }
#ifdef DEBUG
    std::cout << "Max encoing length is " << max_len << std::endl;
#endif
    return max_len;
  }
 
};


//---------------------------------------------------------canonical huff word decoder
//forwod declare
template<typename _KeyType>
class CanonicalHuffDecoder;

template<>
class CanonicalHuffDecoder<std::string> : public Decoder<std::string> {
public:
  CanonicalHuffDecoder(const std::string& infile_name, std::string& outfile_name)
      : Decoder<std::string>(infile_name, outfile_name), 
        infile_name_(infile_name), outfile_name_(outfile_name){}
   
//TODO 所有这些一起序列化？how?
//   * 1. symbol num             1 int 
//   * 2. max encoding length    1 int
//   * 3. min encoding length    1 int
//   * 4. symbol       array     symbol num
//   * 5. symbol_index array     max_len + 1
//   * 6. first_code   array     max_len + 1
//   * 7. start_pos    array     max_len + 1

  //read header init all symbol_[] start_pos_[] first_code_[]
  //actually get encode info and decode file
  void get_encode_info()
  {
    std::ifstream fin(infile_name_.c_str(), std::ios::binary);//把对象写到file.txt文件中
    boost::archive::binary_iarchive ia(fin);   //文本的输出归档类，使用一个ostream来构造
   

    for (int i = 0; i < 2; i++) {
      ia >> symbol_num_[i] >> max_len_[i] >> min_len_[i];
      symbol_[i].resize(symbol_num_[i]);
      symbol_index_[i].resize(symbol_num_[i]);
#ifdef DEBUG
      std::cout << endl;
      std::cout << "symbol num is " << symbol_num_[i] << std::endl;
      std::cout << "max encoding length is " << max_len_[i] << std::endl;
      std::cout << "min encoding length is " << min_len_[i] << std::endl;
#endif
      for (int j = 0; j < symbol_num_[i]; j++) {
        ia >> symbol_[i][j];
      }
     

      for (int j = 0; j < symbol_num_[i]; j++) {
        ia >> symbol_index_[i][j];
      }
      
      for (int j = 0; j <= max_len_[i]; j++) {
        ia >> first_code_[i][j];
      }     
       
      for (int j = 0; j <= max_len_[i]; j++) {
        ia >> start_pos_[i][j];
      }
     
    }

      
    //----read header ok give control to FILE*
    long long pos = fin.tellg();  //makr the position
    fin.seekg (0, std::ios::end);
    fin.close();
    fseek (this->infile_, pos , SEEK_SET);
  }

  //empty for decompressor adpat, will decode int the
  //last part of get encode info
  void decode_file() 
  {
    Buffer reader(this->infile_); 
    Buffer writer(this->outfile_);
#ifdef DEBUG
    std::cout << "decode in canonical huff word" << std::endl;
#endif  

    //--------------------------------------decode each byte
    unsigned char c;
    int v = 0;        //like a global status for value right now
    int len = 0;      //like a global status for length
    std::string symbol;
    std::string end_mark;
    end_mark.push_back(-1);
    
    
    int now = 0;
    int other = 1;
    while(1) { 
      reader.fast_read_byte(c);
      std::bitset<8> bits(c);
      for (int i = 7; i >= 0; i--) {
        v = (v << 1) | bits[i];
        len++;                //length add 1
        if (v >= first_code_[now][len]) {  //OK in length len we translate one symbol
          symbol = symbol_[now][ symbol_index_[now][ start_pos_[now][len] + v - first_code_[now][len]]];
          if (symbol == end_mark) {  //end of file mark!
#ifdef DEBUG
            std::cout << "meetting the coding end mark\n";
#endif          
            writer.flush_buf();
            fflush(this->outfile_);
            return;
          }
          
          writer.write_symbol_string(symbol);
          v = 0;
          len = 0;        //fished one translation set to 0
          std::swap(now, other); //word nonword word nonword.....
        }
      }
    }
  }
protected:
  //unsigned int symbol_[CharSymbolNum];         //symbol <= 257
  //may be [0] for word if word encode first or [0] for nonword if nonword first
  unsigned int symbol_num_[2];
  std::vector<unsigned int> symbol_index_[2];
  unsigned int start_pos_[2][64];       //encoding max length less than 32,so need 33 is ok,give 64 TODO may > 32?
  unsigned int first_code_[2][64];
  std::vector<std::string> symbol_[2];  //TODO vector will slow down decoding speed?FIXME

  unsigned int min_len_[2];
  unsigned int max_len_[2];       //max encoding length

  std::string infile_name_;
  std::string outfile_name_;
};

}  //----end of namespace glzip

#endif  //----end of CANONICAL_HUFFWORD_H_
