/** 
 *  ==============================================================================
 * 
 *          \file   tokenizer.h
 *
 *        \author   chenghuige@gmail.com
 *          
 *          \date   2009-11-27 12:19:57.261802
 *  
 *   Description:   1. This is a tokenizer specialized
 *                  for canonical huffman word based method,
 *                  for segmenting word and non word.
 *                  2. I will try to make it more general
 *                  ie iterator,can work with boost::tokenizer
 *                  and can work with Chinese character TODO
 *
 *                  TODO 字符串的操作还可以优化吧，速度瓶颈在哪，
 *                  string是如何实现的,c++的多线程，例如4.4.2
 *                  中多线程的sort,boost asio thread, ACE
 *
 *  ==============================================================================
 */

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <string>
#include<tr1/unordered_map>
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif
namespace std
{
 using namespace __gnu_cxx;
}
#include <vector>
#include <map>
#include <iomanip>   //for setw format output


namespace glzip {

//the simplest right now just
//Main point:
//   split between alpha,num and sapce,punct
//Add below 
//   1. he's  is a word     he''s is not a word   Done
//   2. pku-online is a word                      Done
//   3. 19821029  to break it into 1982 empty_nonword 1029    TODO 
//   4. big word like aadfsdfdsfdsfdsf exceed 15 character will break TODO
//   5. for dict purpose His turn to his                      TODO
//   TODO try char* instead of string and learn more about string implementaion

inline 
bool ishypen(unsigned char c) {
  if (c =='\''|| c == '-' || c == '_')
    return true;
  return false;
}

/**
 * This Tokenizer will store word to the user offered wcontainer_
 * store non word to the user offered ncontainer_
 * container are of type HashMap
 *
 * For user input
 * Iterator first_ and last_ will mark the input range
 * WordFunc and NonWordFun be the functions type the user want
 * to deal with the splited word and nonword
 */ 
template<
   typename Iterator,                                   //iterator type
   typename WordFunc, typename NonWordFunc = WordFunc,  //func type
   typename Type = std::string                          //token type
>
class Tokenizer {
public:
  //FIXME now I suppose for word or non word each symbol will not ocur > 4G times
  //typedef std::tr1::unordered_map<std::string, unsigned int>   HashMap;
  //typedef std::map<std::string, unsigned int>   HashMap;
  //typedef std::hash_map<std::string, long long, hash_string> HashMap;

  Tokenizer(Iterator first, Iterator last, 
            const WordFunc& wfunc = WordFunc(), 
            const NonWordFunc& nfunc = NonWordFunc())
     : first_(first), last_(last),
       deal_word(wfunc), deal_nonword(nfunc),
       word_first_(false), word_last_(false){}

  //init withot input range, wating for reset
  Tokenizer(){}

  bool is_word_first() const {
    return word_first_;
  }

  bool is_word_last() const {
    return word_last_;
  }
  //for huffman purpose do not clear container 
  //we want to deal with differnt files but 
  //caculating status as one
  void reset(Iterator first, Iterator last,
             const WordFunc& wfunc = WordFunc(), 
             const NonWordFunc& nfunc = NonWordFunc()) {
    first_ = first;
    last_ = last;
    deal_word = wfunc;
    deal_nonword = nfunc;
    word_first_ = false;
    word_last_ = false;
    wtoken_.clear();
    ntoken_.clear();
  }

  void split() {
    //the first symbol will be word otherwise will be non word
    if (std::isalnum(*first_)) 
      word_first_ = true;
 
    for (; first_ != last_;  ++first_) {
      do_split(first_);  //process one character
    }
    
    if (!wtoken_.empty()) {
      deal_word(wtoken_);
      wtoken_.clear();
      word_last_ = true;  //the last one is a word
    }

    if (!ntoken_.empty()) {
      deal_nonword(ntoken_);
      ntoken_.clear();
    }
  }


  /**
   * Split the word and non word,
   * making sure word and non word are seprated
   * one bye one 
   */
  void do_split(Iterator& iter) {
    //is a num 0 1  or alph like a b
    if (std::isalnum(*iter)) {  
      if (!ntoken_.empty()) {
        //deal with - he's
        if (ntoken_.size() == 1 && ishypen(ntoken_[0])) { 
          wtoken_.push_back(ntoken_[0]);
          ntoken_.clear();
        } 
        else {
          //find a non word token
          deal_nonword(ntoken_);
          ntoken_.clear();
        }
      }
      wtoken_.push_back(*iter);
    }
    //non word character like space
    else {     
      if (!wtoken_.empty() && 
          ( !(ishypen(*iter) && ntoken_.empty()))) {
        //find a word token
        deal_word(wtoken_);
        wtoken_.clear();
      }
      ntoken_.push_back(*iter);
    }
  }
  
private:
  Iterator     first_;
  Iterator     last_;

  WordFunc     deal_word;
  NonWordFunc  deal_nonword;
  
  Type         wtoken_;      //word token 
  Type         ntoken_;      //non word token

  bool         word_first_;  //the start is a word or not
  bool         word_last_;   //the end is a word or not
};


}   //end of namespace glzip
#endif  //----end of TOKENIZER_H_
