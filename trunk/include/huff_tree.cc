#define HUFF_TREE_CC_

#include <bitset>
#include "huff_tree.h"

//FIXME how make your code more general for eaxmple for Chinese character file,
//may be 2 bytes as a symbol is more acceptable!! that means 16k enoding table
namespace glzip{

//--------------------------------------------------------------HuffTree
//
#ifdef DEBUG2
/**
* Print the huff tree,the subtree from root
* using pygrahviz
* so depend on graphviz pygraphviz python boost.python
* The result is written to the result_file
*/
template <typename _KeyType>
void HuffTree<_KeyType>::print(std::string result_file)
{
  using namespace boostpy;
  if(!root())
    return;

  std::cout << "Printing the hufftree to the file " << std::endl; 

  Py_Initialize();
  
  object main_module = import("__main__");
  object main_namespace = main_module.attr("__dict__");

  exec("import pygraphviz as pgv", main_namespace);
  exec("tree_graph = pgv.AGraph(directed=True,strict=True)", main_namespace);
  object tree_graph = main_namespace["tree_graph"];
  
  long long key_num = 1;    //key num of node from 1 to ++
  long long invs_num = -1;  //key num of invisable node from -1 to --

  do_print(root_, tree_graph, key_num, invs_num);

  exec("tree_graph.graph_attr['epsilon']='0.001'", main_namespace);
  exec("tree_graph.layout('dot')", main_namespace);
  //write the result
  tree_graph.attr("write")(result_file); 
}


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
template <typename _KeyType>
long long HuffTree<_KeyType>::
do_print(Node* node, boostpy::object &tree_graph, 
    long long &key_num, long long &invs_num)
{
  using namespace boostpy;
  str c = str(key_num);
  long long local_key_num = key_num; //save on local copy of key_num right now

  //-----------------------------------------------leaf 
  //first only print key for leaf TODO add weight
  if (node->is_leaf()) {
    //NOTICE if you use str key = str(node->key(())) the node->key() 
    //say 'a' will be treated as num ie 47 so you will finally get "47"
    //using lexical_cast it's cool you will get std::string "a"
    std::string key = boost::lexical_cast<std::string>(node->key());
    if (node->key() == '\n') 
      key = "nl";
    else if (node->key() == ' ')
      key = "space";
    str weight = str(node->weight());

    tree_graph.attr("add_node")(*make_tuple(c), 
        **dict(make_tuple(make_tuple("label", key+"\\n"+weight), make_tuple("shape", "rect"),
                          make_tuple("color", "blue"))));
    return local_key_num;  
  }
  
  //----------------------------------------------interanl node
  tree_graph.attr("add_node")(*make_tuple(c), 
      **dict(make_tuple(make_tuple("label", str(node->weight())))));
  
  //---------------------------recur of the left
  //hufftree must have lef child for non leaf, create and mark edge as 0
  str l = str(do_print(node->left(), tree_graph, ++key_num, invs_num));
  
  tree_graph.attr("add_edge")(*make_tuple(c, l),
      **dict(make_tuple(make_tuple("label", str(0)))));
  
  //----------------------------rec of the right
  //hufftree must have right child for non leaf, create and mark edge as 1
  str r = str(do_print(node->right(), tree_graph, ++key_num, invs_num));

  tree_graph.attr("add_edge")(*make_tuple(c, r),
      **dict(make_tuple(make_tuple("label", str(1)))));

  //-----------------------------deal with invisable node
  //to make tree more balanced for view
  str inv = str(invs_num--);
  
  tree_graph.attr("add_node")(*make_tuple(inv),
      **dict(make_tuple(make_tuple("style","invis"))));
  
  tree_graph.attr("add_edge")(*make_tuple(c, inv),
      **dict(make_tuple(make_tuple("style","invis"))));
  
  boost::python::list param;
  param.append(l);  //TODO how to better write?
  param.append(r);
  param.append(inv);
  //make_list(l, r, inv) ? TODO can not find make_list now
  //TODO is there a make_list? Where is it how to include?
  
  //int python #tree_graph.add_subgraph([l, r, inv], rank = "same")
  object sub_tree_graph = 
    tree_graph.attr("add_subgraph")(*make_tuple(param),
        **dict(make_tuple(make_tuple("rank","same"))));
  
  sub_tree_graph.attr("add_edge")(*make_tuple(l, inv),
      **dict(make_tuple(make_tuple("style","invis"))));
  
  sub_tree_graph.attr("add_edge")(*make_tuple(inv, r),
      **dict(make_tuple(make_tuple("style","invis"))));


  return local_key_num; 
}
#endif      //---------------end of #ifdef DEBUG2
//----------------------------------------------------------EncodeHuffTree
template <typename _KeyType>
void EncodeHuffTree<_KeyType>::build_tree()
{
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


//TODO try to use char[256] to speed up!
template <typename _KeyType>
void EncodeHuffTree<_KeyType>::
do_gen_encode(Node* root, std::string& encode)
{
 if (root->is_leaf()) {
   encode_map_[root->key()] = encode;
   return;
 }
 encode.push_back('0');              //TODO how string operation is implemented what is the effecience??
 do_gen_encode(root->left(), encode);
 encode[encode.size() - 1] = '1';
 do_gen_encode(root->right(), encode);
 encode.erase(encode.size() - 1, 1);
}


//serialize like (1, 1), (1, 1), (0, 'a')....
template <typename _KeyType>
void EncodeHuffTree<_KeyType>::
do_serialize_tree(Node* root, Buffer& writer)
{
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

//---------------------------------------------------DecodeHuffTree
///*byte based method*/
template <typename _KeyType>
void DecodeHuffTree<_KeyType>::decode_file()
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

///* bit based method, it works however is slow TODO FIXME:( */
//template <typename _KeyType>
//void DecodeHuffTree<_KeyType>::decode_file()
//{
//  Buffer writer(outfile_);
//  unsigned char left_bit, last_byte;
//  reader_.read_byte(left_bit);
//  reader_.read_byte(last_byte);
//  //--------------------------------------decode each byte
//  int bit;
//  Node* cur_node = root();
//  while (reader_.read_bit(bit)) 
//    decode_bit(bit, writer, cur_node);
//  //--------------------------------------deal with the last byte
//  if (left_bit) {
//    std::bitset<8> bits(last_byte);
//    for (int i = 7; i >= left_bit; i--) 
//      decode_bit(bits[i], writer, cur_node);
//  }
//  writer.flush_buf();
//  fflush(outfile_);
//}

/*since slow Buffer removed support for read bit, decode_bit
 *TODO may try to use BitBuffer
 * */
//template <typename _KeyType>
//void DecodeHuffTree<_KeyType>::
//decode_bit(int bit, Buffer& writer, Node*& cur_node)
//{
//  if (bit)
//    cur_node = cur_node->right_;
//  else
//    cur_node = cur_node->left_;
//
//  if (cur_node->is_leaf()) {
//    writer.write_byte(cur_node->key_);
//    cur_node = root();
//  }
//}

template <typename _KeyType>
void DecodeHuffTree<_KeyType>::
decode_byte(unsigned char c, Buffer& writer, Node*& cur_node, int bit_num)
{
  std::bitset<8> bits(c);
  int end = 8 - bit_num;
  for (int i = 7; i >= end; i--) {
    if (bits[i])
      cur_node = cur_node->right_;
    else
      cur_node = cur_node->left_;
  
    if (cur_node->is_leaf()) {
      writer.write_byte(cur_node->key_);
      cur_node = root();
    }
  }
}


//help debug to see if the tree rebuild from file is the same as the intial one
template <typename _KeyType>
void DecodeHuffTree<_KeyType>::
do_gen_encode(Node* root, std::string& encode)
{
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


template <typename _KeyType>
void DecodeHuffTree<_KeyType>::
do_build_tree(Node*& root)
{
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


}   //end of space glzip


