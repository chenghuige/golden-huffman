/** 
 *  ==============================================================================
 * 
 *          \file   test.cc
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-13 18:11:09.645440
 *  
 *   Descriptioni:  应用google test对 glzip compressor decompressor进行正确性测试
 *                  会对我加入的所有的压缩解压缩算法进行测试,正确性通过最终
 *                  对比初始的文件,和经过压缩再解压缩后得到的文件是否完全一致检测.
 *                  同时google test默认会显示每个test的运行时间,部分性能检验
 *
 *                  当前只是测试了一个big.log 24M 的文件 
 *  ==============================================================================
 */
#include <stdio.h>
#include <iostream>

#include "buffer.h"
#include "normal_huff_encoder.h"
#include "compressor.h"

#include <gtest/gtest.h> //using gtest make sure it is first installed on your system

using namespace std;
using namespace glzip;

string infile_name("big.log");
string outfile_name;
string infile_name2, outfile_name2;

//------------------------------*Step1 is to compress a file,perf test!
TEST(huff_char_compress, perf)
{
  Compressor<> compressor(infile_name, outfile_name);
  compressor.compress();
}

//------------------------------*Step2 is to decompress the file compressed in step1,perf test!
TEST(huff_char_decompress, perf)
{
  infile_name2 = outfile_name;
  Decompressor<> decompressor(infile_name2, outfile_name2);
  decompressor.decompress();
}

//------------------------------*Step3 is to see if the final file(after compress and decompress)
//-----------------------------------is the same as the original one.Functional test! 
TEST(huff_char, func)
{
  int num1, num2;
  num1 = num2 = 0;
  FILE* p_orignal;
  FILE* p_final;
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
    EXPECT_EQ(key1, key2) << key1 << " " << key2 << " " 
                          << "differ index is " << num1 ;   
    num1++;
    num2++;
  }
  while(reader2.read_byte(key2))
    num2++;
  //确保初始文件和最终文件的大小一样
  EXPECT_EQ(num1, num2) << "file size differ " 
                        << "original " << num1 
                        << "final " << num2;     
  fclose(p_orignal);
  fclose(p_final);
}

int main(int argc, char *argv[])
{
  if (argc == 2) {  //user input infile name
    infile_name = argv[1];
  }
  testing::GTEST_FLAG(output) = "xml:";
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}




