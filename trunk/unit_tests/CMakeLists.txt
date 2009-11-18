include_directories (${PROJECT_SOURCE_DIR}/include)

set (utest_srcs test.cc)
add_executable (utest ${utest_srcs})

target_link_libraries(utest libboost_python.so libpython2.6.so)
