cmake_minimum_required(VERSION 3.16)
project(DistKV CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

include_directories(include)

add_library(kv_core
    src/store.cpp
    src/wal.cpp
    src/parser.cpp
    src/thread_pool.cpp 
)

add_executable(distkv src/server.cpp)
target_link_libraries(distkv kv_core pthread)

add_executable(bench tests/bench.cpp)
target_link_libraries(bench pthread)