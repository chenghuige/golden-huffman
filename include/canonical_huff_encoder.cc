#define CANONICAL_HUFF_ENCODER_CC_
#include "canonical_huff_encoder.h"

namespace glzip {

//get encoding length 
//space using is 
//frequence map 256  long long
//encoding map  256  string
//length        256  int
//group         256  int
//queue         256  int
void CanonicalHuffEncoder<unsigned char>::
get_encoding_length()
{
  int group[256];  
  HuffNodeIndexGreater index_cmp(this->frequency_map_);
  HuffPRQUE queue(index_cmp);
#ifdef DEBUG
  FrequencyHashMap freq_map_copy;
  std::copy(this->frequency_map_, 
            this->frequency_map_ + 256, freq_map_copy);
#endif 
  //------init queue
  for (int i = 0 ; i < 256 ; i++) { 
    if (this->frequency_map_[i])
      queue.push(i);
    group[i] = -1;
    length_[i] = 0;
  }
  //------imitate creating huff tree using array
  int top_index1,top_index2, index;
  int times = queue.size() - 1;
  for (int i = 0 ; i < times ; i++) {
    top_index1 = queue.top();
    queue.pop();
    top_index2 = queue.top();
    queue.pop();

    index = top_index2;
    //the node group of top_index1 all add 1
    while(group[index] != -1) {  
      length_[index] += 1;
      index = group[index];
    }
    //link group of top_index1 to the group top_index2
    group[index] = top_index1;
    //the node group of top_index2 all add 1
    while(index != -1) {
      length_[index] += 1;
      index = group[index];
    }      
    //now chage weight of frequency_map_[top_index2] representing the
    //new internal node of node top_index1 + node top_index2
    this->frequency_map_[top_index2] += this->frequency_map_[top_index1];

    queue.push(top_index2);
  }
#ifdef DEBUG
  std::copy(freq_map_copy, 
            freq_map_copy + 256, this->frequency_map_);
#endif 
}


}  //----end of namespace glzip

