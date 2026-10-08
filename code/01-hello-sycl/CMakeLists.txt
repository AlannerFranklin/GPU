# 需要先安装 Intel oneAPI Base Toolkit（见 SETUP.md 第 4 节）。
# 构建方式：
#   build.bat 01-hello-sycl run        （本脚本会自动调用 icpx）
# 或者手动：
#   icx-cl -fsycl main.cpp -o hello_sycl.exe   （Windows + MSVC 环境）

cmake_minimum_required(VERSION 3.20)
project(hello_sycl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_executable(hello_sycl main.cpp)

# 优先使用 Intel DPC++ 编译器（icpx），它才是真正能把 kernel 编译到 GPU 上的编译器。
# 如果找不到，退回到普通编译器并给出明确警告——这时程序会跑在 CPU 上。
find_program(ICX_CL NAMES icx-cl icx HINTS
    "$ENV{ONEAPI_ROOT}/compiler/latest/bin"
    "C:/Program Files (x86)/Intel/oneAPI/compiler/latest/bin")

if(ICX_CL)
    message(STATUS "找到 Intel DPC++ 编译器: ${ICX_CL}")
    set_target_properties(hello_sycl PROPERTIES CXX_COMPILER "${ICX_CL}")
else()
    message(WARNING
        "未找到 Intel DPC++ 编译器 (icpx/icx-cl)。\n"
        "程序仍可编译，但 kernel 会跑在 CPU 上，失去 GPU 练习的意义。\n"
        "请安装 Intel oneAPI Base Toolkit，或先运行:\n"
        "  C:\\Program Files (x86)\\Intel\\oneAPI\\setvars.bat")
endif()

set_target_properties(hello_sycl PROPERTIES
    COMPILE_OPTIONS "-fsycl"
    LINK_OPTIONS "-fsycl")
