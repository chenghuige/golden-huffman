set (utest_srcs test.cc)
#add_executable (utest ${utest_srcs})
#target_link_libraries(utest libboost_python.so libpython2.6.so)

set (test_tokenizer_srcs test_tokenizer.cc)
#add_executable (test_tokenizer ${test_tokenizer_srcs})
#target_link_libraries(test_tokenizer libboost_filesystem.so)

set (test_huffword_srcs test_huffword.cc)
add_executable(test_huffword ${test_huffword_srcs})
target_link_libraries(test_huffword libboost_filesystem.so libboost_serialization.so)




