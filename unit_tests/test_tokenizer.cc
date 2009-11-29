/** 
 *  ==============================================================================
 * 
 *          \file   test_tokenizer.cc
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-27 12:05:21.327442
 *  
 *   Description:
 *
 *  ==============================================================================
 */

#include <iostream>
#include <fstream>
using namespace std;
#include <gtest/gtest.h> 
#include "buffer.h"

#include "tokenizer.h"

#ifdef DEBUG
#define protected public  //for debug
#define private   public
#endif

#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h> //using gtest make sure it is first installed on your system
using namespace std;
using namespace glzip; //Notice it is must be after ,using namesapce is dangerous so be carefull

//TODO using class for google test
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <vector>

namespace bfs = boost::filesystem;

string infile_name("simple.log");
string top_path_name;

template<typename T>
void print_result(ofstream& out, T& vec1, T& vec2) 
{
  int i;
  for (i = 0; i < vec2.size(); i++) {
    out << vec1[i] << vec2[i];
  }
  if (i < vec1.size())
    out << vec1[i];
}

void test_tokenizer(const string& filename = infile_name,
    const string& top_path_name = top_path_name) 
{
   typedef std::tr1::unordered_map<std::string, unsigned int>   HashMap;
   HashMap word_container;
   HashMap nword_container;

  if (!top_path_name.empty()) {
    bfs::path topPath(top_path_name);
     if (!bfs::exists(topPath))  {
       cout << "not exist" << endl;
       return;
    }
    bfs::directory_iterator dirIter, endIter;
    try  {
        dirIter = bfs::directory_iterator(topPath);
    }
    catch (bfs::filesystem_error& err)  {
        // We cannot traverse this directory, mark it with a dashed line
        std::cerr << "Error: " << err.what() << std::endl;
    }

    vector<string> file_vec;
    for ( ; dirIter != endIter; ++dirIter)  {
        if (is_regular_file(*dirIter)) {
          bfs::path filepath = topPath / dirIter->leaf();
          cout << filepath.native_directory_string() << endl;
          file_vec.push_back(filepath.native_directory_string());
        }
    }

    typedef istreambuf_iterator<char>  Iter;
    typedef Tokenizer<HashMap,Iter> Tokenizer;
    Tokenizer tokenizer(word_container, nword_container);
    for (int i = 0; i < file_vec.size(); i++) {
      cout << file_vec[i] << endl;
      ifstream ifs(file_vec[i].c_str());
      Iter iter(ifs);
      Iter end;
      tokenizer.reset(iter, end);
      tokenizer.split();
      cout << tokenizer.wcontainer_.size() << " " << tokenizer.ncontainer_.size() << endl;
    }
    ofstream wordlog("word.log");
    ofstream nonwordlog("nonword.log");
    tokenizer.print(wordlog, nonwordlog);
  }
  else {
    ifstream ifs(filename.c_str());
    typedef istreambuf_iterator<char>  Iter;
    Iter iter(ifs);
    Iter end;
  
    typedef Tokenizer<HashMap, Iter> Tokenizer;
    Tokenizer tokenizer(word_container, nword_container, iter, end);
    tokenizer.split();
    //tokenizer.print();
    word_container[""] = 1;
    nword_container[""] = 1;
    cout << tokenizer.wcontainer_.size() << " " << tokenizer.ncontainer_.size() << endl;
  #ifdef DEBUG2 
    cout << tokenizer.wvec_.size() << " " << tokenizer.nvec_.size() << endl;
    ofstream ifs2("out.log");
    int i;
    if (tokenizer.word_first_) 
      print_result(ifs2, tokenizer.wvec_, tokenizer.nvec_);  
    else
      print_result(ifs2, tokenizer.nvec_, tokenizer.wvec_);
  #endif
    }
}

TEST(tokenizer, perf)
{
  test_tokenizer();
}


int main(int argc, char *argv[])
{  
  if (argc == 2) {  //user input infile name
    infile_name = argv[1];
  }

  if (argc == 3)
    top_path_name = argv[2];

  //testing::GTEST_FLAG(output) = "xml:";
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
