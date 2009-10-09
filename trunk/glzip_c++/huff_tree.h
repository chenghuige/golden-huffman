/* 
 * Here use one class for both internal node and leaf.
 * This is easier to implement but will cost more space.
 *
 * For the implementation of differnting internal and leaf
 * refering to "A practical introduction to data structure
 * and algorithm analisis p 115"
 * */
#ifndef _HUFF_TREE_H_
#define _HUFF_TREE_H_  //TODO  automate this for .h file

#include "type_traits.h"
#include "buffer.h"
#include "assert.h"
#include <queue>
#include <deque>
#include <vector>
#include <functional>
#include <iostream>
namespace glzip{
//---------------------------------------------HuffNode--------------
template <typename _KeyType>
struct HuffNode {
  typedef HuffNode<_KeyType>      Node;
  ///allow default construct
  HuffNode() {}
  ///construct a new leaf for character key or string key
  HuffNode(_KeyType key, size_t weight = 0) 
      : key_(key), weight_(weight),
        left_(NULL), right_(NULL){}
  
  ///construct a internal leaf from two child
  //HuffNode(const HuffNode* lchild, const HuffNode* rchild)  //TODO from const to non const fail
  //HuffNode(HuffNode* lchild, HuffNode* rchild) 
  //    : left_(lchild), right_(rchild), 
        //weight_(lchild->weight_ + rchild->weight){}
  HuffNode(HuffNode* lchild, HuffNode* rchild) 
      : left_(lchild), right_(rchild) {
    weight_ = lchild->weight() + rchild->weight();
  }

 
  _KeyType key() const {
    return key_;
  }

  size_t weight() const {
    return weight_;
  }

  Node* left() const {
    return left_;
  }

  Node* right() const {
    return right_;
  }

  bool is_leaf() {
    return !left_;  //left_ is NULL means right_ is also,for huf tree it is a full binary tree,every internal node is of degree 2
  }
  
  /////The comparison operator used to order the priority queue.
  ////-----But I choose to use the func object for storing pointer in the queuq
  ////-----not the Node it's self, TODO see the performance differnce 
  //bool operator > (const HuffNode& other) const {
  //  return weight > other.weight;
  //}
  
  //-------------------------------------------------------------------
  _KeyType  key_;
  size_t    weight_;   //here weight is frequency of char or string 
  Node*     left_;
  Node*     right_;
};

//-----------------------------------------------HuffTree--------------
/**
 * For HuffTree
 * It take the frequency_map_ and encode_map_ as input.
 * Those two are not owned by HuffTree but HuffEncoder.
 * HuffTree will use frequence_map_ info to make encode_map_ ok.
 * 1. Wait until frequency_map_ is ready (which is handled by HuffEncoder)
 * 2. build_tree()  
 * 3. gen_encode()
 * 4. serialize_tree()
 * The sequence is important can not break!
 *
 * TODO(Array based HuffTree) actually the hufftree can be implemented using simple array do
 * not need building the tree.
 *
 * TODO For string type the tree might be so big, the rec is OK?
 * */
template <typename _KeyType>
class HuffTreeBase {
public:
  typedef HuffNode<_KeyType>                                        Node;
public:
  void set_root(Node* other) {
    root_ = other;
  }

  void delete_tree(Node* root) { //TODO rec what if the tree is so big?
    if (root) {
      delete_tree(root->left());
      delete_tree(root->right());
      delete root;
    }
  }
 
  //for test
  void travel(Node* root) {
    if (root) {
      travel(root->left());
      travel(root->right());
    }
  }

  Node* root() const {
    return root_;
  }
protected:
  Node*   root_;
};

template <typename _KeyType, typename _TreeType = encode_hufftree>
class HuffTree: public HuffTreeBase<_KeyType> {
public:
  using HuffTreeBase<_KeyType>::root;
  using HuffTreeBase<_KeyType>::set_root;
  using HuffTreeBase<_KeyType>::delete_tree;
  //typedef this->root_   root_;

  typedef typename TypeTraits<_KeyType>::type_catergory             type_catergory;
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap           FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap              EncodeHashMap;
  typedef HuffNode<_KeyType>                                        Node;
  
  struct HuffNodePtrGreater:
      public std::binary_function<const Node *, const Node *, bool> {
    
    bool operator() (const Node *p1, const Node *p2) {
      //return p1->weight_ >  p2->weight_;
      return p1->weight() > p2->weight();
    }
  };
  //typedef std::deque<Node> HuffDQU;   //TODO use vector to see which is better and why
  //typedef std::priority_queue<Node,HuffDQU, greater<Node> >         HuffPRQUE; //desending order use less<HuffNode> if asending
  typedef std::deque<Node*> HuffDQU;   //TODO use vector to see which is better and why
  typedef std::priority_queue<Node*, HuffDQU, HuffNodePtrGreater>     HuffPRQUE; //desending order use less<HuffNode> if asending

public:
  HuffTree(EncodeHashMap& encode_map, FrequencyHashMap& frequency_map)  //long long int (&)[256] can not be inited by const long ...
        : encode_map_(encode_map), frequency_map_(frequency_map) {} //can not build when initing because frequency_map is not ready! TODO
  ~HuffTree() {
    //std::cout << "dstruct hufftree\n";
    delete_tree(root());
  }
  void gen_encode() {
    std::string encode;
    do_gen_encode(root(), encode);   
    //std::cout << "Finished encoding\n";
  }
  void build_tree() {
    init_queue(type_catergory());
    int times = pqueue_.size() - 1;
    for (int i = 0; i < times; i++) {
      Node* lchild = pqueue_.top();
      pqueue_.pop();
      Node* rchild = pqueue_.top();
      pqueue_.pop();
      Node* p_internal = new Node(lchild, rchild);
      pqueue_.push(p_internal);
    }
    set_root(pqueue_.top());  
    //std::cout << "Finished building tree\n"; 
  }

  ///write the header info to the outfile, for decompressor to rebuild the tree
  //----write in pre order travelling
  void serialize_tree(FILE* outfile) {
    Buffer writer(outfile);  //the input outfile cur should be at 0
    do_serialize_tree(root(), writer);
    writer.flush_buf();   //make sure writting to the file
  }
private:
  void init_queue(char_tag) {
    for(int i = 0; i < 256; i++) {
      if (frequency_map_[i]) {
        Node* p_leaf = new Node(i, frequency_map_[i]); //key is i and weight is frequency_map_[i]
        pqueue_.push(p_leaf);        //push leaf
      }
    }
  }

  //TODO try to use char[256] to speed up!
   void do_gen_encode(Node* root, std::string& encode) {
    if (root->is_leaf()) {
      encode_map_[root->key()] = encode;
      return;
    }
    encode.append("0");              //TODO how string operation is implemented what is the effecience??
    do_gen_encode(root->left(), encode);
    encode[encode.size() - 1] = '1';
    do_gen_encode(root->right(), encode);
    encode.erase(encode.size() - 1, 1);
  }

  //void do_gen_encode(Node* root, std::string encode) {
  //  //if (root->is_leaf()) {
  //  //if (root->left() == NULL || root->right() == NULL) {
  //  //  //encode_map_[root->key()] = encode;
  //  //  return;
  //  //}
  //  if (!root->right() && !root->left())
  //    return;
  //  do_gen_encode(root->left(), encode + "0");
  //  do_gen_encode(root->right(), encode + "1");
  //}


  //serialize like (1, 1), (1, 1), (0, 'a')....
  void do_serialize_tree(Node* root, Buffer& writer) {
    if (root->is_leaf()) {
      writer.write_byte(0);  //0 means the leaf
      writer.write_byte(root->key());  //write the key
      return;
    }
    writer.write_byte(255);  //255 means the internal node
    writer.write_byte(255);  //any num is ok
    do_serialize_tree(root->left(), writer);
    do_serialize_tree(root->right(), writer);
  }

  //----------------------------------------------------for string_tag--------
  void init_queue(string_tag) {
    
  }
private:
  HuffPRQUE              pqueue_;
  EncodeHashMap&         encode_map_;
  FrequencyHashMap&      frequency_map_;
};

/** Specitialized HuffTree for decoding*/
template <typename _KeyType>
class HuffTree<_KeyType, decode_hufftree>
    : public HuffTreeBase<_KeyType>{
public:
  using HuffTreeBase<_KeyType>::root;
  using HuffTreeBase<_KeyType>::root_;
  using HuffTreeBase<_KeyType>::set_root;
  using HuffTreeBase<_KeyType>::delete_tree;
  typedef HuffNode<_KeyType>  Node;

public:
  HuffTree(FILE* infile, FILE* outfile)
      :infile_(infile), outfile_(outfile),
       reader_(infile) {} 

  ~HuffTree() {
    delete_tree(root());
  }

  ///build_tree() is actually get_encode_info()
  void build_tree() {  //From the infile header info we can build the tree
    do_build_tree(root_);
  }
 
  //help debug to see if the tree rebuild from file is the same as the intial one
  void do_gen_encode(Node* root, std::string& encode) {
    if (root->is_leaf()) {
      std::cout << root->key() << " " << encode << "\n";
      return;
    }
    encode.append("0");              //TODO how string operation is implemented what is the effecience??
    do_gen_encode(root->left(), encode);
    encode[encode.size() - 1] = '1';
    do_gen_encode(root->right(), encode);
    encode.erase(encode.size() - 1, 1);
  }

  void decode_file() 
  {
    Buffer writer(outfile_);
    unsigned char left_bit, last_byte;
    reader_.read_byte(left_bit);
    reader_.read_byte(last_byte);
    //--------------------------------------decode each byte
    Node* cur_node = root();
    unsigned char c;
    while(reader_.read_byte(c)) 
      decode_byte(c, writer, cur_node);
    //--------------------------------------deal with the last byte
    if (left_bit)
      decode_byte(last_byte, writer, cur_node, (8 - left_bit));
      writer.flush_buf();
    fflush(outfile_);
  }

private:
  void do_build_tree(Node*& root) {
    unsigned char first, second;
    reader_.read_byte(first);
    reader_.read_byte(second);
    if (first == 0) {  //is leaf  TODO actually we do not need weight this time so HuffNode can be smaller
      root = new Node(second);
      return;
    }
    root = new Node();
    do_build_tree(root->left_);
    do_build_tree(root->right_);
  }

  void decode_byte(unsigned char c, Buffer& writer, Node*& cur_node, int bit_num = 8) 
  {
    unsigned char mask = 128; //1 << 7
    for (int i = 0; i < bit_num; i++) {
      if ((c & mask) == 0)  //--------------bit i of c is 0,turn left
        cur_node = cur_node->left_;
      else
        cur_node = cur_node->right_;
      mask >>= 1;
      if (cur_node->is_leaf()) {
        writer.write_byte(cur_node->key_);
        cur_node = root();
      }
    }
  }
private:
  FILE*  infile_;
  FILE*  outfile_;
  Buffer reader_;   //need reader_ becuause for two function build_tree and decode_file we need the same reader
};

}   //end of space glzip
#endif  //------------end of _HUFF_TREE_H_
