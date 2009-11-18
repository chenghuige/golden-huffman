#ifndef _BUFFER_H_
#define _BUFFER_H_

namespace glzip {

/**
 * Buffer
 * Provide buffered read and write
 * wrapped fread() and fwrite(). 
 */
class Buffer {
public:
  Buffer(FILE* file, int buf_capacity = 64 * 1024) 
      :file_(file), cur_(0), bit_cur_(0), num_(0), 
       buf_capacity_(buf_capacity), buf_used_(0) {
    buf_ = new unsigned char[buf_capacity];
  }
  ~Buffer() {
    delete [] buf_;
  }
  
  int fill_buf() {
    cur_ = 0;
    buf_used_ = fread(buf_, 1, buf_capacity_, file_);
    return buf_used_;
  }
  
  void flush_buf() {
    fwrite(buf_, 1, cur_, file_);
    cur_ = 0;
  }

  int read_byte(unsigned char& c) {
    if (cur_ == buf_used_ && fill_buf() == 0) 
      return 0;   //at the end, did not read
    c = buf_[cur_++];
    return 1;     //successfully read one byte
  }

  ///TODO using read bit will slow down the speed?
  //int read_bit(int& x) {
  //  if (bit_cur_ == 0 && read_byte() == 0)
  //    return 0;
  //  bit_cur_ = (bit_cur_ + 1) % 8;
  //}

  void write_byte(unsigned char c) {
    if (cur_ == buf_capacity_)
      flush_buf();
    buf_[cur_++] = c;
  }

  //x must be 0 or 1
  void write_bit(int x) {
    num_ = ((num_ << 1) + x);
    if ((++bit_cur_) == 8) {  //++bit_cur_ not bit_cur_++ fixed bug here
      write_byte((unsigned char)(num_));  //write byte
      num_ = 0;
      bit_cur_ = 0;
    }
  }
  
  //for writing string or bits,the last byte,
  //return how many bits still need to fill
  int left_bits() {
    return (8 - bit_cur_) % 8;
  }

 
  //s must be like "00111010"
  void write_string(const std::string& s) {
    for (unsigned int i = 0; i < s.size(); i++) 
      write_bit(s[i] - '0');
  }

private:
  FILE* file_;
  int cur_;
  int bit_cur_;  //for bit handling, 0 - 8 write process
  int num_;      //for bit handling  write process
  int buf_capacity_;
  int buf_used_;
  unsigned char* buf_;
};

}   //end of namespace glzip
#endif  //----end of _BUFFER_H_

