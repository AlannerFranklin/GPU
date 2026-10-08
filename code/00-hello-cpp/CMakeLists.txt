cmake_minimum_required(VERSION 3.20)
project(hello_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release 是学 GPU 的唯一合理模式：Debug 下的性能数字毫无意义
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_executable(hello_cpp main.cpp)

# MSVC: 开警告、优化、生成调试信息（Release 下也要，方便 profiling）
#   /W4              警告等级 4
#   /O2              优化
#   /Zi              生成 PDB，Release 下也要，否则 profiler 看不到符号
#   /utf-8           源码按 UTF-8 解析，避免中文注释乱码
#   /Zc:__cplusplus  不加这个，MSVC 会把 __cplusplus 报成 199711（即 C++98）
if(MSVC)
    target_compile_options(hello_cpp PRIVATE /W4 /O2 /Zi /utf-8 /Zc:__cplusplus)
endif()

find_package(Threads REQUIRED)
target_link_libraries(hello_cpp PRIVATE Threads::Threads)
