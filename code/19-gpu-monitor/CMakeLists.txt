cmake_minimum_required(VERSION 3.24)
project(gpu_monitor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

add_definitions(-DUNICODE -D_UNICODE)
add_executable(pdh_demo pdh_demo.cpp)
target_link_libraries(pdh_demo pdh)