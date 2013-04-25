cmake_minimum_required(VERSION 2.8)
PROJECT (cserver)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -ggdb -Wall -Wno-non-virtual-dtor -Wno-strict-aliasing -Wno-unused-value -pedantic -Wcast-align -Wcast-qual -Winit-self -Wuninitialized -Wno-long-long -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -Wwrite-strings -Wparentheses -pthread -rdynamic")

add_subdirectory(src)


