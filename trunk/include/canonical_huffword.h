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
#ifdef DEBUG2
    std::ofstream ofs("f.log");
    print_frequency(frequency_map_[0], ofs);
    ofs.clear();
#endif
    get_encoding_length(frequency_map_[0]);
#ifdef DEBUG2
    std::ofstream ofs2("l.log");
    print_length(frequency_map_[0], ofs2);
    ofs2.close();
#endif

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


  /**write header*/
  void write_encode_info()
  {

  }

  void encode_file()
  {

  }
private:
  /**cacluate each symbol's enoding length*/
  //can be a function out of class
  struct greater_pointer {
    
    bool operator()(unsigned int __x, unsigned int  __y) const { 
      return *(reinterpret_cast<unsigned int*>(__x)) > *(reinterpret_cast<unsigned int*>(__y)); 
    }
  };

  /*
   * The method is similar as in MG book,p47
   * uisng additional n size space only
   */
  void get_encoding_length(HashMap& hmp)
  {
    //we use heap_array first to store pointers point to 
    //hashp map frequency then will conver to use as int
    //so make sure sizeof(int*) == sizeof(int)! 
    const int ksize = hmp.size();
    unsigned int heap_array[ksize];
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

      //the new one we insert is (unsigned int *)&(reinterpret_cast<int>(heap_array[ksize - i - 1]))
      //the heap will shrank size 1 only each loop
      //TODO why can not match shift down?
      //can use vec shift_down but for array not need difference type FIXME learn more

      glzip::heap_shift_down(heap_array, heap_array + ksize - i - 1, 
                             reinterpret_cast<unsigned int>(&(heap_array[ksize - i - 1])),
                             greater_pointer()
                            );
      //glzip::shift_down(heap_array, 0, ksize - i - 1, 
      //                  (unsigned int)reinterpret_cast<unsigned int>(&(heap_array[ksize - i - 1])), 
      //                  greater_pointer());
      

      *m1 = ksize - i - 1;  //store index will be ok, pointer from 2 son node to parent
      *m2 = ksize - i - 1;
    }
    //--------------------------------------------------calc length
    //child  length = it's parent length + 1
    heap_array[1] = 0;
    for (int i = 2; i < ksize; i++) {
      heap_array[i] = heap_array[heap_array[i]] + 1;
    }
   
    for (iter = hmp.begin(); iter != end; ++iter) {
      (*iter).second.fl.length = heap_array[(*iter).second.fl.length] + 1;
    }

  }

};


}  //----end of namespace glzip

#endif  //----end of CANONICAL_HUFFWORD_H_
