/** 
 *  ==============================================================================
 * 
 *          \file   test_huffword.cc
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-28 19:25:17.702542
 *  
 *   Description:
 *
 *  ==============================================================================
 */

#include <iostream>
using namespace std;
#include <gtest/gtest.h> 
#include <stdio.h>
#include <iostream>
#include <string>
#include "buffer.h"

#ifdef DEBUG
#define protected public  //for debug
#define private   public
#endif

#include "compressor.h"  
#include "canonical_huff_encoder.h"
#include "canonical_huffword.h"
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>

#include <gtest/gtest.h> //using gtest make sure it is first installed on your system
using namespace std;
using namespace glzip; //Notice it is must be after ,using namesapce is dangerous so be carefull

//TODO using class for google test

string infile_name("simple.log");
string outfile_name;
string infile_name2, outfile_name2;
/* 
 * compressor  and decompressor  is normal huffman char based method
 * compressor2 and decompressor2 is canonical huffman char based method
 * */
Compressor<CanonicalHuffEncoder<std::string> > compressor;

void func_test() 
{
  int num1, num2;
  num1 = num2 = 0;
  FILE* p_orignal;
  FILE* p_final;
  
  cout << "Test pass if the two files is of the same content" << endl;
  cout << "Original file is " << infile_name << endl;
  cout << "Final file is " << outfile_name2 << endl;

  p_orignal = fopen(infile_name.c_str(), "rb");
  p_final = fopen(outfile_name2.c_str(), "rb");
  //p_final = fopen("big.log2", "rb");  //one failure test 

  Buffer reader(p_orignal);
  Buffer reader2(p_final);
  unsigned char key1, key2;
  while(reader.read_byte(key1)) {
    reader2.read_byte(key2);
    //确保初始文件,和压缩然后再解压缩后的文件内容一致
    //对应的每一个byte都一致 
    ASSERT_EQ(key1, key2) << key1 << " " << key2 << " " 
                          << "differ index is " << num1 ;   
    num1++;
    num2++;
  }
  while(reader2.read_byte(key2))
    num2++;
  //确保初始文件和最终文件的大小一样
  ASSERT_EQ(num1, num2) << "file size differ " 
                        << "original " << num1 
                        << "final " << num2;     
  fclose(p_orignal);
  fclose(p_final);
}


void test_compress() 
{
  outfile_name.clear();
  compressor.set_file(infile_name, outfile_name);
  compressor.compress();
}

void test_decompress()
{
  infile_name2 = outfile_name;
  outfile_name2.clear();
  Decompressor<CanonicalHuffDecoder<std::string> > decompressor(infile_name2, outfile_name2);
  decompressor.decompress();
}

void test_fast_decompress()
{
  infile_name2 = outfile_name;
  outfile_name2.clear();
  Decompressor<FastCanonicalHuffDecoder<std::string> > decompressor(infile_name2, outfile_name2);
  decompressor.decompress();
}

void test_table_decompress()
{
  infile_name2 = outfile_name;
  outfile_name2.clear();
  Decompressor<TableCanonicalHuffWordDecoder > decompressor(infile_name2, outfile_name2);
  decompressor.decompress();
}


TEST(compress, perf)
{
  test_compress();
}

TEST(decompress, perf)
{
  test_decompress();
}

TEST(result, func)
{
  func_test(); 
}

TEST(fast_decompress, perf)
{
  test_fast_decompress();
}

TEST(fast_result, func)
{
  func_test(); 
}

TEST(table_decompress, perf)
{
  test_table_decompress();
}

TEST(table_result, func)
{
  func_test(); 
}


int main(int argc, char *argv[])
{
  if (argc == 2) {  //user input infile name
    infile_name = argv[1];
  }

  //testing::GTEST_FLAG(output) = "xml:";
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
