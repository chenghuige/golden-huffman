/** 
 *  ==============================================================================
 * 
 *          \file   type_traits.h
 *
 *        \author   pku_goldenlock@qq.com
 *
 *          \date   2009-11-18 16:12:13.356033
 *  
 *   Description:   Class HuffTree and HuffNode
 *                  
 *               
 *                  EncodeHuffTree
 *                  is the huff tree for compressing process
 *
 *                  DecodeHuffTree
 *                  is the huff tree for decompressing process
 *  ==============================================================================
 */

/* 
 * Here use one class for both internal node and leaf.
 * This is easier to implement but will cost more space.
 *
 * For the implementation of differnting internal and leaf
 * refering to "A practical introduction to data structure
 * and algorithm analisis p 115"
 * */
#ifndef HUFF_TREE_H_
#define HUFF_TREE_H_  //TODO  automate this for .h file

#include "type_traits.h"
#include "buffer.h"
#include "assert.h"
#include <queue>
#include <deque>
#include <vector>
#include <functional>
#include <iostream>

#ifdef DEBUG2
#include <boost/python.hpp> //for using boost.python to print the hufftree
#include <boost/lexical_cast.hpp> //type conversion

namespace boostpy = boost::python; //for tree printing
#endif

namespace glzip{
//----------------------------------------------------------------------------HuffNode------
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
  //TODO from const to non const fail
  HuffNode(HuffNode* lchild, HuffNode* rchild) 
      : left_(lchild), right_(rchild) {
    weight_ = lchild->weight() + rchild->weight();
  }

  _KeyType key() const{
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

//-----------------------------------------------HuffTree-------------------HuffTreeBase----
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
class HuffTree {
public:
  typedef HuffNode<_KeyType>       Node;
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
  
  Node* root() const {
    return root_;
  }

  //---------------------for test
  //pre order travel to see if the tree is correct
  void travel(Node* root) {
    if (root) {
      travel(root->left());
      travel(root->right());
    }
  }

#ifdef DEBUG2
  /**
   * Print the huff tree,the subtree from root
   * using pygrahviz
   * so depend on graphviz pygraphviz python boost.python
   * The result is written to the result_file
   */
  void print(std::string result_file = "huff_tree.dot");
private:
  //Return key index num of node
  //Using c,l,r p as the
  //c --- current,l ----left ,r --- right
  //inv --- invisable
  //Using post travel frame work when at c ,it's left part l
  //and right part r are OK,only need to add edge c->l and c->r
  //Notice return value is nonzero!
  //Note it is a binary tree pringing function specialized for
  //hufftree which do not need to use invisiable node for one left or right null child!
  //For a normal sence binary tree may need to write another one.
  long long do_print(Node* node, boostpy::object &tree_graph, 
    long long &key_num, long long &invs_num);
#endif

protected:
  Node*   root_;
};

//---------------------------------------------------------------------------HuffTree for encode---
template <typename _KeyType>
class EncodeHuffTree: public HuffTree<_KeyType> {
public:
  using HuffTree<_KeyType>::root;
  using HuffTree<_KeyType>::set_root;
  using HuffTree<_KeyType>::delete_tree;

  typedef typename TypeTraits<_KeyType>::type_catergory             type_catergory;
  typedef typename TypeTraits<_KeyType>::FrequencyHashMap           FrequencyHashMap;
  typedef typename TypeTraits<_KeyType>::EncodeHashMap              EncodeHashMap;
  typedef HuffNode<_KeyType>                                        Node;
  
  struct HuffNodePtrGreater:
      public std::binary_function<const Node *, const Node *, bool> {
    
    bool operator() (const Node *p1, const Node *p2) {
      return p1->weight() > p2->weight();
    }
  };
  //typedef std::deque<Node> HuffDQU;   //TODO use vector to see which is better and why
  //typedef std::priority_queue<Node,HuffDQU, greater<Node> >         HuffPRQUE; //desending order use less<HuffNode> if asending
  typedef std::deque<Node*> HuffDQU;   //TODO use vector to see which is better and why
  typedef std::priority_queue<Node*, HuffDQU, HuffNodePtrGreater>     HuffPRQUE; //desending order use less<HuffNode> if asending

public:
  //long long int (&)[256] can not be inited by const long ...
  EncodeHuffTree(EncodeHashMap& encode_map, FrequencyHashMap& frequency_map)          
    : encode_map_(encode_map), frequency_map_(frequency_map) 
  {  
    build_tree();        //assmue frequency_map is ready when creating the tree        
  }  
  ~EncodeHuffTree() {
    //std::cout << "dstruct hufftree\n";
    delete_tree(root());
  }
  void gen_encode() {
    std::string encode;
    do_gen_encode(root(), encode);   
    //std::cout << "Finished encoding\n";
  }
  void build_tree();
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
  void do_gen_encode(Node* root, std::string& encode);

  //serialize like (1, 1), (1, 1), (0, 'a')....
  void do_serialize_tree(Node* root, Buffer& writer);
  //----------------------------------------------------for string_tag--------
  void init_queue(string_tag) {
    
  }
private:
  HuffPRQUE              pqueue_;
  EncodeHashMap&         encode_map_;
  FrequencyHashMap&      frequency_map_;
};

//---------------------------------------------------------------------------HuffTree for decode---
template <typename _KeyType>
class DecodeHuffTree: public HuffTree<_KeyType>{
public:
  using HuffTree<_KeyType>::root;
  using HuffTree<_KeyType>::root_;
  using HuffTree<_KeyType>::set_root;
  using HuffTree<_KeyType>::delete_tree;
  typedef HuffNode<_KeyType>  Node;

public:
  DecodeHuffTree(FILE* infile, FILE* outfile)
      :infile_(infile), outfile_(outfile),
       reader_(infile) {} 

  ~DecodeHuffTree() {
    delete_tree(root());
  }

  ///build_tree() is actually get_encode_info()
  void build_tree() {  //From the infile header info we can build the tree
    do_build_tree(root_);
  }
 
  //help debug to see if the tree rebuild from file is the same as the intial one
  void do_gen_encode(Node* root, std::string& encode);
  void decode_file();

private:
  void do_build_tree(Node*& root);
  
  void decode_byte(unsigned char c, Buffer& writer, Node*& cur_node, int bit_num = 8);

  void decode_bit(int bit, Buffer& writer, Node*& cur_node);

private:  
  FILE*  infile_;
  FILE*  outfile_;
  Buffer reader_;   //need reader_ becuause for two function build_tree and decode_file we need the same reader
};

}   //end of space glzip
#ifndef HUFF_TREE_CC_
#include "huff_tree.cc"
#endif
#endif  //------------end of HUFF_TREE_H_
