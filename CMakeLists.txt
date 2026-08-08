cmake_minimum_required(VERSION 3.16)
project(rmalloc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)

add_library(rmalloc INTERFACE)
target_include_directories(rmalloc INTERFACE include/)

add_executable(demo src/demo.cpp src/rmalloc.cpp)
target_link_libraries(demo PRIVATE rmalloc Threads::Threads)

# bench calls rmalloc()/rmfree() directly — no operator new override
add_executable(bench src/bench.cpp)
target_link_libraries(bench PRIVATE rmalloc Threads::Threads)
target_compile_options(bench PRIVATE -O2)