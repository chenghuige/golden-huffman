#define ENCODER_CC_
#include "encoder.h"

#include <typeinfo>   //type id
#include <functional>
#include <numeric>
#include <iomanip>   //for setw 输出格式控制

namespace glzip {

template<typename _KeyType>
void Encoder<_KeyType>::do_encode_file(char_tag)
{
  long pos = ftell(outfile_);      //The pos right after the head info of compressing
  Buffer writer(outfile_);
  writer.write_byte((unsigned char)(0));      //at last will store leftbit here
  writer.write_byte((unsigned char)(0));      //at last will store the last byte if leftbit > 0
  
  fseek (infile_ , 0 , SEEK_SET ); //Cur of infile_ to the start,we must read it again
  Buffer reader(infile_); 
  
    //TODO one poosible way is to let the while be virtual so 
  //we will not call so many virtual functions in while
  encode_each_byte(reader, writer);
 
  writer.flush_buf();   //important! need to write all the things int the buf out even buf is not full
  //---deal with the last byte
  int left_bits = writer.left_bits();
  //std::cout << "left bits when encoding is " << left_bits << "\n";
  if (left_bits) {
      //fseek(outfile_, 0, pos);   //fixed bug here,went back to the saved pos;  why this is wrong????TODO
      fseek(outfile_, pos, SEEK_SET);
      writer.write_byte((unsigned char)(left_bits));
      for (int i = 0; i < left_bits; i++)
        writer.write_bit(0);    //fill 0 to finish the byte 
      writer.flush_buf();       //write buf to file
  }
  fflush(outfile_);    //force to the disk
}

template<typename _KeyType>
void Encoder<_KeyType>::do_print_encode(char_tag, std::ostream& out)
{
  using namespace std;
  out << "The input file is " <<  infile_name_ << "\n";
  //-----统计文件中一共有多少个byte文件大小
  long long start = 0;
  long long byte_sum =std::accumulate(frequency_map_, frequency_map_ + 256, start);
  out << "The total bytes num is " << byte_sum << "\n";
  out << "\n";
  //-----统计文件中一共出现了多少个不同的字符byte
  //-----统计平均码长,忽略头部信息的压缩率 
  //TODO gcc4.5 支持lamada 否则用foreach之类去外面定义仿函数和函数得不偿失
  //boost lamada?
  //TODO c++0x Stream Fusion 避免多次循环? 现在提出来多一个循环,影响效率
  //但是代码更清楚,一个循环一个单独的功能,主要还是能够为了先显示字符数目
  //long long character_sum = std::count_if
  long long character_sum = 0;
  float avg_length = 0;  //in average how many bits are needed for one character after encode ,before encode is 8
  for (int i = 0; i < 256; i++) {
    if (frequency_map_[i]) {
      character_sum++;
      avg_length += encode_map_[i].length() * frequency_map_[i];
    }
  }
  avg_length /= float(byte_sum);

  out << "The total number of different characters in the file is " 
      << character_sum << "\n";
  out << "The average encoding length per character is " << avg_length << "\n";
  out << "So not consider the header the approximate compressing ration should be " 
      << avg_length/8.0 << "\n"; 
  out << "\n";
  //-----打印编码信息
  out << "The encoding map:\n\n";
  
  out << setiosflags(ios::left) << setw(20) << "Character"
                                << setw(20) << "Times"
                                << setw(20) << "Frequence" 
                                << setw(20) << "EncodeLength"
                                << setw(30) << "Encode" << "\n\n";
  for (int i = 0; i < 256; i++) {
    if (frequency_map_[i]) {
      //左对齐,占位
      unsigned char key = i;
      if (key == '\n') {
       out << setiosflags(ios::left) << setw(20) << "\\n"  
                              << setw(20) << frequency_map_[i]
                              << setw(20) << setprecision(3) << frequency_map_[i]/(float)byte_sum
                              << setw(20) << encode_map_[i].length()
                              << setw(30) << encode_map_[i] << "\n";
      } else if (key == ' ') {
        out << setiosflags(ios::left) << setw(20) << "space" 
                              << setw(20) << frequency_map_[i]
                              << setw(20) << setprecision(3) << frequency_map_[i]/(float)byte_sum
                              << setw(20) << encode_map_[i].length()
                              << setw(30) << encode_map_[i] << "\n";
      } else {
        out << setiosflags(ios::left) << setw(20) << key
                              << setw(20) << frequency_map_[i]
                              << setw(20) << setprecision(3) << frequency_map_[i]/(float)byte_sum
                              << setw(20) << encode_map_[i].length()
                              << setw(30) << encode_map_[i] << "\n";
      }
    }
  }
  //out.close() //not close for std::ostream struct std::basic_ostream<char, std::char_traits<char> >
}


}  //----end of namespace glzip

