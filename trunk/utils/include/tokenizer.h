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
 *                  can work with Chinese character
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
 * Iterator first_ and last_ will mark the input range
 */ 
template<
   typename HashMap = std::tr1::unordered_map<std::string, unsigned int>,
   typename Iterator = std::string::const_iterator,
   typename Type = std::string
>
class Tokenizer {
public:
  //FIXME now I suppose for word or non word each symbol will not ocur > 4G times
  //typedef std::tr1::unordered_map<std::string, unsigned int>   HashMap;
  //typedef std::map<std::string, unsigned int>   HashMap;
  //typedef std::hash_map<std::string, long long, hash_string> HashMap;

  Tokenizer(HashMap& wcontainer, HashMap& ncontainer, Iterator first, Iterator last)
     : first_(first), last_(last), word_first_(false),
       wcontainer_(wcontainer), ncontainer_(ncontainer){}

  //init withot input range, wating for reset
  Tokenizer(HashMap& wcontainer, HashMap& ncontainer)
    :wcontainer_(wcontainer), ncontainer_(ncontainer){}

  void clear() {
    wtoken_.clear();
    ntoken_.clear();
#ifdef DEBUG2
    nvec_.clear();
    wvec_.clear();
#endif
  }

  //for huffman purpose do not clear container 
  //we want to deal with differnt files but 
  //caculating status as one
  void reset(Iterator first, Iterator last) {
    first_ = first;
    last_ = last;
    word_first_ = false;
    clear();
  }

  void add_nonword(Type& nword) {
    ncontainer_[nword] += 1;
#ifdef DEBUG2
    nvec_.push_back(nword);
#endif 
  }

  void add_word(Type& word) {
    wcontainer_[word] += 1;
#ifdef DEBUG2
    wvec_.push_back(word);
#endif 
  }

  void split() {
    //the first symbol will be word otherwise will be non word
    if (std::isalnum(*first_)) 
      word_first_ = true;
 
    for (; first_ != last_;  ++first_) {
      do_split(first_);  //process one character
    }
    
    if (!wtoken_.empty()) {
      add_word(wtoken_);
      wtoken_.clear();
    }

    if (!ntoken_.empty()) {
      add_nonword(ntoken_);
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
          add_nonword(ntoken_);
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
        add_word(wtoken_);
        wtoken_.clear();
      }
      ntoken_.push_back(*iter);
    }
  }
  

  void print(std::ostream& out = std::cout,
             std::ostream& out2 = std::cout) 
  {
    using namespace std;
    typedef typename HashMap::iterator Iter;
    Iter end = wcontainer_.end();

    for (Iter iter = wcontainer_.begin();iter != end; ++iter) {
      out << setiosflags(ios::left) 
          << setw(20) << iter->first 
          << setw(20) << iter->second << endl;
    }
    end = ncontainer_.end();
    for (Iter iter = ncontainer_.begin();iter != end; ++iter) {
      out2 << setiosflags(ios::left) 
           << setw(20) << iter->first 
           << setw(20) << iter->second << endl;
    }
  }
private:
  Iterator first_;
  Iterator last_;
  
  Type     wtoken_;      //word token 
  Type     ntoken_;      //non word token

  bool     word_first_;
  
  HashMap&  wcontainer_; //word container
  HashMap&  ncontainer_; //nonword container

#ifdef DEBUG2
  std::vector<std::string> wvec_; //word vector
  std::vector<std::string> nvec_; //non word vector
#endif
  
};


}   //end of namespace glzip
#endif  //----end of TOKENIZER_H_
