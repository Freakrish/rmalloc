cmake_minimum_required(VERSION 3.16)
project(rmalloc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(rmalloc INTERFACE)
target_include_directories(rmalloc INTERFACE include/)

add_executable(demo src/demo.cpp)
target_link_libraries(demo PRIVATE rmalloc)