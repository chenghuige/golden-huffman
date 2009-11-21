#define ENCODER_CC_
#include "encoder.h"

#include <typeinfo>   //type id
#include <functional>
#include <numeric>
#include <iomanip>   //for setw 输出格式控制

namespace glzip {

//the process frame work will mainly handle 
//the left bits
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
  //different encoder will use differnt method
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



}  //----end of namespace glzip

