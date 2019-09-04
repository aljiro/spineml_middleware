cmake_minimum_required (VERSION 2.6)
project (mbmiddleware)

find_package (Threads)
add_executable(mbmiddleware main.cpp mbmiddleware.cpp mbmiddleware.h)
target_link_libraries (mbmiddleware ${CMAKE_THREAD_LIBS_INIT})

