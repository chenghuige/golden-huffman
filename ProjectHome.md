C++ implementaion of multiple compressing method, using template.

# 计划 #
  * 完成一个C++实现各种常用文本压缩算法的库.
  * 会尽可能的优化性能和减少占用空间。采用模板类template增强复用，如支持基于char和基于word的huffman,范式huffman算法(spporting both char and word based
  * 强调实验的特性，会详细输出中间过程，如打印具体的huffan tree.

# 进展 #
```
11.30
完成了英文文本基于分词的范式huffman完全无损的压缩解压缩。
对于24M的一个测试英文文本用普通的基于字节的压缩可压缩到13M，
而基于分词的压缩当前测试是9.5M,gzip默认选项压缩到7.6M
如果改进分词或者是对于更大的英文文本(这个测试文本中符号比较多稍微影响效果)
基于词的压缩能取得更好的效果。
下一步，改进分词，改进速度，尝试中文分词压缩，或者混合文本...

golden_huffman1.1
Table based canonical huff decoding is quick.
allen:~/study/data_structure/golden-huffman/build/bin$ du -h 5big.log
116M    5big.log
1.Normal canonical huff decoding(char based)
allen:~/study/data_structure/golden-huffman/build/bin$ time ./utest 5big.log.crs2 4

real    0m5.287s
user    0m2.500s
sys     0m2.572s
2.Table based canonical huff decoding(char based)  
allen:~/study/data_structure/golden-huffman/build/bin$ time ./utest 5big.log.crs2 6

real    0m3.621s
user    0m2.132s
sys     0m1.428s
3.gzip decoding
allen:~/study/data_structure/golden-huffman/build/bin$ time gzip -d 5big.log.gz

real    0m5.970s
user    0m1.868s
sys     0m1.532s

  golden_huffman1.0
  * Finished character(byte) normal huffman compressing decomressing method.
  * Using boost.python, pygraphviz printing the huff tree to dot file.
  * Finshed character(byte) canonical huffman compressing and decompressing.
  * TODO try to optimize decompressing process of byte canonical huffman.
  * TODO dictionary,word based huffman and canonical huffman.

allen:~/study/data_structure/golden-huffman/build/bin$ time ./utest
[==========] Running 2 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 2 tests from canonical_huff_char
[ RUN      ] canonical_huff_char.compress_perf
[       OK ] canonical_huff_char.compress_perf (1037 ms)
[ RUN      ] canonical_huff_char.decomress_perf
[       OK ] canonical_huff_char.decomress_perf (912 ms)
[----------] 2 tests from canonical_huff_char (1950 ms total)

[----------] Global test environment tear-down
[==========] 2 tests from 1 test case ran. (1951 ms total)
[  PASSED  ] 2 tests.

real    0m1.975s
user    0m1.280s
sys     0m0.676s
allen:~/study/data_structure/golden-huffman/build/bin$ du -h big.log
24M     big.log
allen:~/study/data_structure/golden-huffman/build/bin$ du -h big.log.crs2
13M     big.log.crs2
allen:~/study/data_structure/golden-huffman/build/bin$ du -h big.log.crs2.de
24M     big.log.crs2.de
allen:~/study/data_structure/golden-huffman/build/bin$ diff big.log big.log.crs2.de
allen:~/study/data_structure/golden-huffman/build/bin$ time gzip big.log

real    0m3.607s
user    0m3.348s
sys     0m0.136s
allen:~/study/data_structure/golden-huffman/build/bin$ du -h big.log.gz
7.9M    big.log.gz
allen:~/study/data_structure/golden-huffman/build/bin$ time gzip -d big.log.gz

real    0m0.742s
user    0m0.228s
sys     0m0.488s
```
![http://golden-huffman.googlecode.com/svn/wiki/tree.jpg](http://golden-huffman.googlecode.com/svn/wiki/tree.jpg)