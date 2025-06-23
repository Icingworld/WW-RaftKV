cmake_minimum_required(VERSION 3.10)
project(ww-rpc VERSION 2.1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third-party)
add_subdirectory(raftkv)
add_subdirectory(example)
