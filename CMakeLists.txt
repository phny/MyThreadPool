cmake_minimum_required(VERSION 3.5)

project(thread_pool)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -fPIC -mavx")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

FIND_PACKAGE(Threads REQUIRED)

aux_source_directory(. src_files)

add_executable(thread_pool ${src_files})

target_link_libraries(thread_pool ${CMAKE_THREAD_LIBS_INIT})

