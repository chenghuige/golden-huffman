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

#ifdef DEBUG
#define protected public  //for debug
#define private   public
#endif

#include "compressor.h"  
#include "normal_huff_encoder.h"
#include "canonical_huff_encoder.h"


#include <gtest/gtest.h> //using gtest make sure it is first installed on your system
using namespace std;
using namespace glzip; //Notice it is must be after ,using namesapce is dangerous so be carefull

//TODO using class for google test

string infile_name("big.log");
string outfile_name;
string infile_name2, outfile_name2;
/* 
 * compressor  and decompressor  is normal huffman char based method
 * compressor2 and decompressor2 is canonical huffman char based method
 * */
Compressor<> compressor;
Compressor<CanonicalHuffEncoder> compressor2;

void compressor_func_test() 
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



//-------------------------------------Testing normal huffman char 
//------------------------------*Step1 is to compress a file,perf test!
//TEST(normal_huff_char, compress_perf)
//{
//    compressor.compress();
//}
//
////TEST(normal_huff_char, compress_perf_calcFreq)
////{
////   compressor.set_file(infile_name, outfile_name);
////   //read file and calc           --done by Encoder
////   compressor.encoder_.caculate_frequency();    
////}
////
////TEST(normal_huff_char, compress_perf_genEncode)
////{
////    //gen encode based on frequnce --done by specific encoder
////    compressor.encoder_.gen_encode(); 
////}
////
////TEST(normal_huff_char, compress_perf_writeEncodeInfo)
////{
////    //write outfile header(encoding info) ---done by specific encoder
////    compressor.encoder_.write_encode_info();
////}
////
////TEST(normal_huff_char, compress_perf_encodeFile)
////{
////    //read infile,translate to outfile,   ---done by Encoder
////    compressor.encoder_.encode_file();         
////}
//
//
//
////------------------------------*Step2 is to decompress the file compressed in step1,perf test!
//TEST(normal_huff_char, decomress_perf)
//{
//  infile_name2 = outfile_name;
//  Decompressor<> decompressor(infile_name2, outfile_name2);
//  decompressor.decompress();
//}
//
////------------------------------*Step3 is to see if the final file(after compress and decompress)
////-----------------------------------is the same as the original one.Functional test! 
//TEST(normal_huff_char, func)
//{
//  compressor_func_test();
//}
//
//-------------------------------------Testing canonical huffman char 
//------------------------------*Step1 is to compress a file,perf test!
TEST(canonical_huff_char, compress_perf)
{
  compressor2.compress();
}

//FIXME file_name problem
TEST(canonical_huff_char, decomress_perf)
{
  infile_name2 = outfile_name;
  Decompressor<CanonicalHuffDecoder> decompressor2(infile_name2, outfile_name2);
  decompressor2.decompress();
}

//#ifdef DEBUG
//TEST(canonical_huff_char, encoding_length_func)
//{
//  //TODO auto? here can?
//  //std::string *normal_length 
//  //  = compressor.encoder_.encode_map_;  //string array
//  long long sum1 = 0, sum2 = 0;
//  long long num1 = 0, num2 = 0;
//  for (int i = 0 ; i < 256 ; i++) {
//    unsigned char key = i;
//    int normal_len = compressor.encoder_.encode_map_[i].length();
//    int canoni_len = compressor2.encoder_.length_[i];
//    long long normal_frq = compressor.encoder_.frequency_map_[i];
//    long long canoni_frq = compressor2.encoder_.frequency_map_[i];
//    EXPECT_EQ(normal_len, canoni_len) << i << key;
//    EXPECT_EQ(normal_frq, canoni_frq) << i << key;
//    //EXPECT_LE(normal_len, 8) << i << key;
//    num1 += normal_frq;
//    num2 += canoni_frq;
//    sum1 += normal_frq * normal_len;
//    sum2 += canoni_frq * canoni_len;
//  }
//  float normal_avg = float(sum1)/num1;
//  float canoni_avg = float(sum2)/num2;
//  EXPECT_FLOAT_EQ(normal_avg, canoni_avg);
//}
//#endif

////------------------------------*Step2 is to decompress the file compressed in step1,perf test!
//TEST(canonical_huff_char, decomress_perf)
//{
//  infile_name2 = outfile_name;
//  Decompressor<> decompressor2(infile_name2, outfile_name2);
//  decompressor2.decompress();
//}
//
////------------------------------*Step3 is to see if the final file(after compress and decompress)
////-----------------------------------is the same as the original one.Functional test! 
//TEST(canonicall_huff_char, func)
//{
//  compressor_func_test();
//}


int main(int argc, char *argv[])
{
  if (argc == 2) {  //user input infile name
    infile_name = argv[1];
  }

  compressor.set_file(infile_name, outfile_name);
  //compressor.clear();
  outfile_name.clear();
  compressor2.set_file(infile_name, outfile_name);
  
  //testing::GTEST_FLAG(output) = "xml:";
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
