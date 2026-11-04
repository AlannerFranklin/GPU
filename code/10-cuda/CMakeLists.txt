cmake_minimum_required(VERSION 3.24)
project(vector_add LANGUAGES CXX CUDA)

# ============================================================
# 编译标准（两台机器统一，约定见 handover/README.md）：
#   - 目标架构 sm_89：家里 RTX 4070 Ti Super (Ada, CC 8.9)
#   - C++17：nvcc 12.6（家）与 13.4（公司）都完整支持
#   - 源码必须以家里的 CUDA 12.6 为底线，禁用 13.x 专有 API
# 新练习复制这份 CMakeLists，只改 project 名和源文件列表。
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES 89)

# 中文注释：MSVC 默认按 GBK 读源文件，/utf-8 防乱码（项目约定，见 SETUP.md）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
# nvcc 12.6 官方支持的 MSVC 上限较低；Build Tools 装的 MSVC 更新时
# nvcc 会报 unsupported compiler，此开关放行（对受支持版本无影响）。
# -Xcompiler 把 /utf-8 传给 MSVC 宿主编译器。
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler -Xcompiler=\"/utf-8\"")

add_executable(vector_add vector_add.cu)
