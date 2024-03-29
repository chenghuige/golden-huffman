/** 
 *  ==============================================================================
 * 
 *          \file   buffer.h
 *
 *        \author   pku_goldenlock@qq.com
 *                  ChengHuige
 *
 *          \date   2009-11-18 16:36:29.568105
 *  
 *   Description:   For providing bufferd fast read and write
 *                  of byte and bit... 
 *  ==============================================================================
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <iostream>
#include <bitset>
#include <memory.h>

#ifdef DEBUG
#define private public
#define protected public
#endif

//FIXME fix machine realted problem with
//read_int write_int BitBuffer
namespace glzip {

/**
 * Buffer
 * Provide buffered read and write
 * wrapped fread() and fwrite(). 
 * especially for 
 * read_byte()  
 * write_byte()
 * write_bit()
 *
 * also:
 * read_int()
 * write_int()
 *
 * Can be only in read mode or write mode
 * at a time.
 *
 * Do not support read_bit since it is slow.
 * Use BitBuffer instead!
 */

//TODO std::size_t N? learn more about std::size_t
//buf_capacity_ should not exceed int capacity

typedef unsigned char UChar;
//forward decalaration
template<typename _Buffer>
class BufReaderIter;

#define IntSize sizeof(int)
template <int buf_capacity_ = 64*1024>
class FixedFileBuffer {
public:
  FixedFileBuffer(FILE* file) 
      :file_(file), cur_(0), 
       bit_cur_(0), 
       num_(0), buf_used_(0),
       end_(0)
  {
    memset(buf_, 0, buf_capacity_ + 8);
    //fill_buf();   //FIXME may be ReaderBuffer and WriterBuffer better TODO 
  }

  FixedFileBuffer() :end_(1) {}
  //------------------------------------------------------iterator support (only for read_byte only)!
  //notice if you use iterator do not use read_byte again!
  
  //typedef BufReaderIter<FixedFileBuffer<buf_capacity_> > iterator;
  //template <typename _Buffer>  friend class BufReaderIter;

  //iterator begin() {
  //  return iterator(this);
  //}

  //iterator end() {
  //  return iterator();
  //}

  //the class it's self is a iterator,just for speed, above is ok but a bit slow
  //usage:
  //FixedFileBuffer<buf_capacity_> rader(input_file);
  //FixedFileBuffer<buf_capacity_>::iterator iter, end;
  //iter = reader.begin()
  //end = reader.end()
  //while(iter != end)
  //  char ch = *iter++;
  typedef FixedFileBuffer<buf_capacity_> iterator;
  iterator begin() {
    fill_buf();
    return *this;
  }

  iterator end() {
    return iterator();
  }

  UChar operator*() const {
    return buf_[cur_];
  }
  
  UChar* operator->() const {
    return buf_ + cur_;
  }

  iterator& operator++() {
    if (cur_ == (buf_used_ - 1) && fill_buf() == 0) 
      end_ = 1;    
    else
      cur_++;
    return *this;
  }

  iterator operator++(int) {
    iterator tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==(const iterator& other) {
    return (end_ == 0 && other.end_ == 0)||(this = &other);
  }

  bool operator!=(const iterator& other) {
    return end_ != other.end_;
  }
  
  //------------------------------------------------------fill and flush buf
  bool is_full() {
    return cur_ == buf_used_;
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

  //-------------------------------------------------------reading

  //if cur == buf_used_ than we read over the buffer
  //than we fill the buf if we fail to read any from file
  //than will return 0 to mark the end
  int read_byte(unsigned char& c) {
    if (cur_ == buf_used_ && fill_buf() == 0) 
      return 0;   //at the end, did not read
    c = buf_[cur_++];
    return 1;     //successfully read one byte
  }

//FIXME template? for the two? if we int n; read_byte(n); we can not use the one above!! must be char&
  int read_byte(int& c) {
    if (cur_ == buf_used_ && fill_buf() == 0) 
      return 0;   //at the end, did not read
    c = (int)buf_[cur_++];
    return 1;     //successfully read one byt
  }
 
  int read_byte(unsigned int& c) {
    if (cur_ == buf_used_ && fill_buf() == 0) 
      return 0;   //at the end, did not read
    c = (unsigned int)buf_[cur_++];
    return 1;     //successfully read one byt
  }

  void fast_read_byte(unsigned char& c) {
    if (cur_ == buf_used_)
      fill_buf(); 
    c = buf_[cur_++];
  }

  //Notice read_int happen in canonical decoding reading header
  //so will not need to fill_buf
  //FIXME may need to check fill_buf(using read_byte) when
  //for word based the header is large or buf_capacity is small!
  //FIXME Need to make sure fill buf first at constructor
  //FIXME read_int and write_int is machine related! int be 32bits!

//FIXME FIXME!
  void read_int(unsigned int& v) {
    //FIXME why if (cur_ + 4 <= buf_used_) wrong?
    //if (cur_ + 4 <= buf_used_)  //there are at leat 4 bytes can read from buffer
    //  fast_read_int(v);
    //else {
      read_byte(v);
      unsigned int v2;
      for (int i = 0; i < (IntSize - 1); i++) {
        read_byte(v2);
        v = v << 8 | v2;
      }
    //}
  }

  //this is for canonical decoder only!! for if using this read_int do not use read_byte
  //because this will have to make sure each read int is on 4 byte alined address
//  void read_int(unsigned int& v) {
//    read_int(v);  //first read in may not fill buf,TODO separate ReaderBuffer and WriterBuffer
//    for (int i = 0; i < (IntSize - 1); i++)
//      v = v << 8 | buf_[cur_++];
//  }


//  void safe_read_int(unsigned int& v) {
//    read_byte(v);
//    unsigned int v2;
//    for (int i = 0; i < (IntSize - 1); i++) {
//      read_byte(v2);
//      v = v << 8 | v2;
//    }
//  }

  void fast_read_int(unsigned int& v) {
    v = buf_[cur_++];  //first read in may not fill buf,TODO separate ReaderBuffer and WriterBuffer
    for (int i = 0; i < (IntSize - 1); i++)
      v = v << 8 | buf_[cur_++];
  }


  //----------------------------------------------writing
  void write_byte(unsigned char c) {
    if (cur_ == buf_capacity_)
      flush_buf();
    buf_[cur_++] = c;
  }

  //x must be 0 or 1
  void write_bit(int x) {
    num_ = ((num_ << 1) | x);
    if ((++bit_cur_) == 8) {  //++bit_cur_ not bit_cur_++ fixed bug here
      write_byte((unsigned char)(num_));  //write byte
      num_ = 0;
      bit_cur_ = 0;
    }
  }
  
  //Notice! used in canonical writting header
  //So do not consider cur >= buf_capacity 
  //buf_ pos   0         1      2      3             4 
  //   int    big_end               small_end
  //FIXME unsigned int not 32bits? Now I think will also work           
  void write_int(unsigned int v) {
    if (cur_ + 4 > buf_used_)  //there are at leat 4 bytes can write from buffer
      flush_buf();
    fast_write_int(v);
  }

  void fast_write_int(unsigned int v) {
    int shift = 8*(IntSize-1);
    for (int i = 0; i < (IntSize - 1); i++) {
      buf_[cur_++] = (v >> shift);
      v <<= 8;
    }
    buf_[cur_++] = (v >> shift);
  }
  //-------------------------------------------special write bit helper for compressor
  //for writing string or bits,the last byte,
  //return how many bits still need to fill
  int left_bits() {
    return (8 - bit_cur_) % 8;
  }

  //make sure we write the last byte!
  void flush_bits() {
    while(left_bits())
      write_bit(1);
  }

  //s must be like "00111010"
  void write_string(const std::string& s) {
    for (unsigned int i = 0; i < s.size(); i++) 
      write_bit(s[i] - '0');
  }

  //bits_num < 32
  //TODO unsigned int?
  void write_bits(unsigned int code, int bits_num) {
    std::bitset<32> bits(code);
    for (int i = (bits_num - 1); i >= 0; i--) {
      write_bit(bits[i]);
    }
  }

  //for huff word
  void write_symbol_string(const std::string& s) {
    for (unsigned int i = 0; i < s.size(); i++) 
      write_byte(s[i]);
  }

private:
  FILE* file_;   //the file we process read or wirte
  int cur_;      //for byte handling
  
  /*for hanling write bit in encoing process*/
  unsigned char bit_cur_;  //for bit handling, 0 - 8 write process
  int num_;      //for bit handling  write process like a bit writter buf:)
  int buf_used_; //the actual number of bytes read from file

  unsigned char buf_[buf_capacity_ + 8]; //the buffer! make 8 more space unused for safety

  bool end_;  //for self iterator
};

typedef FixedFileBuffer<> Buffer;

template<typename _Buffer = Buffer>
class BufReaderIter
{
private:
  UChar    current_;
  _Buffer* preader_;      //TODO must be *
public:
  BufReaderIter() : preader_(NULL) {}
  BufReaderIter(_Buffer* reader) : preader_(reader) {
    if (!preader_->fill_buf())  //empty file
      preader_ = NULL;
    preader_->read_byte(current_);  //for the first *iter
  }
  //TODO UChar& better?  than UChar?
  //TODO why can not *() const?
  //错误： 将类型为‘glzip::UChar&’的引用初始化为类型为‘const glzip::UChar’的表达式无效
  //UChar& operator*() {
  //  return current_;
  //}

  UChar operator*() const {
    return current_;
  }
  
  UChar* operator->() const {
    return &current_;
  }

  BufReaderIter& operator++() {
    if (!preader_->read_byte(current_))
      preader_ = NULL;
    return *this;
  }

  BufReaderIter operator++(int) {
    BufReaderIter tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==(const BufReaderIter<_Buffer>& other) {
    return preader_ = other.preader_;
  }

  bool operator!=(const BufReaderIter<_Buffer>& other) {
    return preader_ != other.preader_;
  }

};

/*
 * BitBuffer
 *
 * Here we use unsigned int to represent a
 * 32 bit, bit buffer.
 *
 * Now use a 64bit buffer or can us 32 bits,
 * similar performance, however these are
 * all machine related,ie 32bit for int  and
 * 64 bit for long long, what if
 * 64 bit for int? FIXME
 * TODO read and understand gzip and mg
 */
#define BitBufferSize  8*sizeof(long long)
#define Shift          BitBufferSize/2
template<typename _Buffer = Buffer>
class BitsBuffer {
public:
  BitsBuffer(Buffer& reader):reader_(reader) {
    fill_buf();
  }

  void fill_buf(unsigned int x) {
    buf_ = x;
    bit_count_ = BitBufferSize;
  }

  void fill_buf() {
    unsigned int v1, v2;
    reader_.read_int(v1);
    reader_.read_int(v2);

    buf_ = (((unsigned long long)(v1)) << (Shift)) | v2 ; 
    bit_count_ = BitBufferSize;
  }

  /**
   * read n bits and return an int which represents the n bits
   * n can be 1 - 32
   */
  unsigned int read_bits(unsigned int n) {
    
    unsigned int x;
    int left_bits;
    
    if (n > bit_count_) {
      //first output what you have
      x = buf_ >> (BitBufferSize - n);
      left_bits = n - bit_count_;
      fill_buf();
      //output the left bits
      x |= buf_ >> (BitBufferSize - left_bits);
      buf_ <<= left_bits;
      bit_count_ -= left_bits;
    }
    else {
      x = buf_ >> (BitBufferSize - n);
      buf_ <<= n;
      bit_count_ -= n;
    }
    return x;
  }

private:
  unsigned int bit_count_;
  unsigned long long buf_;
  _Buffer&  reader_;             //TODO better arrage here
};

typedef BitsBuffer<> BitBuffer;

//#define BitBufferSize 32
//class BitBuffer {
//public:
//  BitBuffer(Buffer& reader):reader_(reader) {
//    fill_buf();
//  }
//
//  void fill_buf(unsigned int x) {
//    buf_ = x;
//    bit_count_ = BitBufferSize;
//  }
//
//  void fill_buf() {
//    reader_.read_int(buf_);
//    bit_count_ = BitBufferSize;
//  }
//
//  /**
//   * read n bits and return an int which represents the n bits
//   * n can be 1 - 32
//   */
//  unsigned int read_bits(unsigned int n) {
//    
//    unsigned int x;
//    int left_bits;
//    
//    if (n > bit_count_) {
//      //first output what you have
//      x = buf_ >> (BitBufferSize - n);
//      left_bits = n - bit_count_;
//      fill_buf();
//      //output the left bits
//      x |= buf_ >> (BitBufferSize - left_bits);
//      buf_ <<= left_bits;
//      bit_count_ -= left_bits;
//    }
//    else {
//      x = buf_ >> (BitBufferSize - n);
//      buf_ <<= n;
//      bit_count_ -= n;
//    }
//    return x;
//  }
//
//private:
//  unsigned int bit_count_;
//  unsigned int buf_;
//  Buffer&  reader_;             //TODO better arrage here
//};

}   //end of namespace glzip
#endif  //----end of BUFFER_H_

