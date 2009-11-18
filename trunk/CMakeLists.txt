cmake_minimum_required (VERSION 2.6)

project (glzip)

#set (CMAKE_CXX_FLAGS -O2)
set(CMAKE_CXX_FLAGS -fpermissive)
set (CMAKE_EXE_LINKER_FLAGS  -lgtest)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

include_directories(/usr/local/boost1.4/include /usr/local/include/python2.6)  
link_directories(/usr/local/boost1.4/lib /usr/local/lib)

#add_subdirectory(utils/src lib)

#add_subdirectory (test bin)

add_subdirectory(unit_tests bin)
