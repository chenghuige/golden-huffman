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
    this->outfile_ = fopen(outfile_name.c_str(), "wb");
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
    //for word hash map
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
    //for non word hash map
    int max_nonword_encoding_length
      = get_encoding_length(frequency_map_[1]);
#ifdef DEBUG
    std::ofstream ofs4("length_nonword.log");
    print_length(frequency_map_[1], ofs4);
    ofs4.close();
#endif

    //really gen encode using length info
    do_gen_encode(frequency_map_[0], max_word_encoding_length);
#ifdef DEBUG
  std::string outfile_name = this->infile_name_ + "_word_canonical_encode.log";
  std::cout << "Writting canonical encode info to " << outfile_name << std::endl;
  std::ofstream out_file(outfile_name.c_str());
  print_encode(frequency_map_[0], out_file);
#endif 
    do_gen_encode(frequency_map_[1], max_nonword_encoding_length);
#ifdef DEBUG
  std::string outfile_name2 = this->infile_name_ + "_nonword_canonical_encode.log";
  std::cout << "Writting canonical encode info to " << outfile_name2 << std::endl;
  std::ofstream out_file2(outfile_name2.c_str());
  print_encode(frequency_map_[1], out_file2);
#endif 
  }
  
  //gen encode for hmp, and will calc min_len also
  static void do_gen_encode(HashMap& hmp, int max_len)
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
    unsigned int first_code[64];      //max_encoding_length should <= 32, we need at most 32+1, now use 64
  
    /** the index position of the min encoing in symbol for each encoding length */
    unsigned int start_pos[64];       

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
#ifdef DEBUG
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
      EXPECT_LT(start_pos_copy[len], SymbolNum) << i << " " << len;
      symbol_index[start_pos_copy[len]++] = i;   //bucket sorting --using index
    }
    
    //------------------------------------at the end we write header
    //write_header(hmp, symbol_index);
  }
 
    static void print_encode(HashMap& hmp, std::ostream& out = std::cout)
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

    static void print_frequency(HashMap& hmp, std::ostream& out = std::cout) 
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
  
  static void print_length(HashMap& hmp, std::ostream& out = std::cout) 
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

  static std::string get_encode_string(unsigned int code, unsigned int len)
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

  static void write_header(HashMap& hmp, unsigned int symbol_index[])
  {
//    //------------------------------------------------OK now can write the header out!
//    //we need to write the 
//    //start_pos  (max_length) ,  we use start_pos_copy which is correct unmodified
//    //first_code (max_length) 
//    //symbol_    (n)
//    const unsigned int SymbolNum = hmp.size();
//    typedef HashMap::iterator Iter;
//    Iter iter = hmp.begin();
//    Iter end = hmp.end();
//
//    fseek (this->outfile_ , 0 , SEEK_SET ); 
//    Buffer writer(this->outfile_);
//#ifdef DEBUG
//    std::cout << "write symbol num is " << SymbolNum << std::endl;
//#endif
//    writer.write_int(SymbolNum); //FIXME do not exceed 4G
//    //FIXME
//    for (unsigned int i = 0; i< SymbolNum; i++, ++iter) 
//      writer.write_int(symbol_index[i]);
//  
//    //write from index 1
//#ifdef DEBUG
//    std::cout << "write max encoding length is " << max_len_ << std::endl;
//#endif
//    writer.write_int(min_len_);
//    writer.write_int(max_len_); 
//    for (int i = 1; i <= max_len_; i++) { //notice start from 1 fixed!!!!
//      writer.write_int(start_pos_[i]);
//      writer.write_int(first_code_[i]);
//    }
//  
//    //encode_file2(writer);
//    writer.flush_buf();
//    fflush(this->outfile_);     //force to the disk! important!
  }
  /**
   * Write header, this is empty since we write encode 
   * actually when at the end of gen encode, becasue we
   * want to pass symbol index array and we want to allocate 
   * symbol index array space only when needed, 
   * so do not as a class data member! using vector as a 
   * data member may be a choice but we want to be as fast
   * as possible
   */
  void write_encode_info()
  {

  }

  void encode_file()
  {

  }
private:
  struct greater_pointer {
    
    bool operator()(unsigned int __x, unsigned int  __y) const { 
      return *(reinterpret_cast<unsigned int*>(__x)) > *(reinterpret_cast<unsigned int*>(__y)); 
    }
  };
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
  static int get_encoding_length(HashMap& hmp)
  {
    //we use heap_array first to store pointers point to 
    //hashp map frequency then will conver to use as int
    //so make sure sizeof(int*) == sizeof(int)! 
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


}  //----end of namespace glzip

#endif  //----end of CANONICAL_HUFFWORD_H_
