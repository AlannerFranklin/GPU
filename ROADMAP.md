# 详细周计划（12 个月）

配套文件：[README.md](README.md)（总纲/两条门）· [SETUP.md](SETUP.md)（环境）· [progress.md](progress.md)（打卡表）

**时间预算假设**：工作日 1.5h/天 + 周末 6h = **约 20 小时/周**。
如果你的实际投入小于 15 小时/周，把整张表按比例拉长，别硬赶进度。

---

## 阶段一：地基（第 1-2 月）

> **这一阶段不碰 GPU。** 看起来慢，但这是整个计划里最重要的一段。
> 你没有 C++ 硬功的话，后面所有 CUDA 学习都会变成"抄代码"。

### 第 1 周 · 环境与「编译到底做了什么」

| 项目 | 内容 |
|---|---|
| **主教材** | CSAPP《深入理解计算机系统》第 3 版 第 1 章、第 7 章（链接） |
| **动手** | 按 [SETUP.md](SETUP.md) 配通本机工具链；跑通 `code/00-hello-sycl` |
| **核心问题** | 一个 `.cpp` 文件从磁盘到进程，中间发生了什么？（预处理→编译→汇编→链接→装载） |
| **练习** | 手写 `main.cpp` + `math_utils.cpp`，用 CMake 编译成静态库/动态库，用 `dumpbin` 看符号表 |
| **产出** | 一个能一键编译的 CMake 项目模板，以后每个练习复用它 |

### 第 2 周 · 指针、内存、C 风格底层

| 项目 | 内容 |
|---|---|
| **主教材** | 《C++ Primer》第 5 版 第 2、3、4、6 章（如果你已熟，直接跳到练习） |
| **核心问题** | 指针和数组的关系？`new/delete` 和 `malloc/free` 的区别？栈和堆？ |
| **练习** | ① 手写 `void* memcpy_impl(void*, const void*, size_t)` ② 手写字符串切分函数（不许用 `std::string`）③ 用 `sizeof` 验证结构体对齐与 padding |
| **自检** | 能画出 `int** p` / `int (*p)[5]` / `int *p[5]` 三者的内存图 |

### 第 3 周 · 类、RAII、拷贝与移动语义

| 项目 | 内容 |
|---|---|
| **主教材** | 《Effective Modern C++》条款 1-17 |
| **核心问题** | 为什么要有移动语义？`std::move` 到底做了什么？三/五法则？ |
| **练习** | **手写一个 `MyString` 类**：构造、析构、拷贝构造、拷贝赋值、移动构造、移动赋值、`operator+`、`operator<<`。要求打印日志验证每一步调用了哪个函数 |
| **产出** | `MyString` 是经典面试题，也是理解 RAII 的最佳载体 |

### 第 4 周 · 模板与 STL 内功

| 项目 | 内容 |
|---|---|
| **主教材** | 《Effective Modern C++》条款 18-30 |
| **核心问题** | 模板什么时候实例化？`std::vector` 扩容策略？迭代器为什么有 5 种？ |
| **练习** | ① 手写 `MyVector<T>`（支持扩容、迭代器、`emplace_back`）② 手写 `unique_ptr` 和 `shared_ptr`（含引用计数）③ 用 `std::sort` + 自定义比较器排 100 万条数据 |
| **产出** | 这两个手写容器是你理解 C++ 内存管理的分水岭 |

### 🔍 M1 月末自检

关掉 AI 和文档，从空白文件开始，2 小时内完成：手写一个 `MyVector<T>`，
支持 `push_back`（含扩容）、`operator[]`、迭代器、`size/capacity`，并通过内存检测无泄漏。

**做不出来 → 第 2 个月补课，不要往下走。**

---

### 第 5 周 · 存储器层次结构（GPU 学习的地基）

| 项目 | 内容 |
|---|---|
| **主教材** | CSAPP 第 6 章（存储器层次结构）—— **这章是理解 GPU 内存模型的钥匙** |
| **核心问题** | 为什么 cache 有效？局部性原理？cache line 是多少字节？ |
| **实验** | CSAPP 的 **Cache Lab**：写一个 cache 模拟器 + 用分块优化矩阵转置 |
| **关键** | ⚠️ 你在这一周学的「分块（blocking）优化矩阵转置」，**就是一个月后 CUDA tiled matmul 的同一件事**。只是 CUDA 里你手动管理片上内存 |

### 第 6 周 · 虚拟内存与进程

| 项目 | 内容 |
|---|---|
| **主教材** | CSAPP 第 9 章（虚拟内存）、第 8 章（异常控制流） |
| **核心问题** | 页表、TLB、缺页中断？进程和线程到底差在哪？系统调用的开销？ |
| **实验** | CSAPP 的 **Malloc Lab**：手写一个 malloc 分配器（分离空闲链表 + 首次适配） |
| **说明** | Malloc Lab 很硬，做不完没关系，但**要真的动手试**。它是理解内存分配器的唯一途径 |

### 第 7 周 · 并发编程

| 项目 | 内容 |
|---|---|
| **主教材** | 《C++ Concurrency in Action》第 1-5 章（中文版《C++ 并发编程实战》） |
| **核心问题** | 数据竞争？`std::atomic` 的内存序（relaxed/acquire/release/seq_cst）？伪共享？ |
| **练习** | ① 用 `std::thread` 并行归约一个数组 ② 用 `std::atomic` 实现自旋锁 ③ 手写一个无锁 SPSC 队列 |
| **关键** | ⚠️ **这周学的东西在 CUDA 里一秒钟都没浪费**。CUDA 的 `__syncthreads()`、
  shared memory 竞争、原子操作，全是同一套心智模型的 GPU 版本 |

### 第 8 周 · 综合项目一：线程池

| 项目 | 内容 |
|---|---|
| **产出** | 一个**工业级线程池**：任务队列、`std::future` 返回值、动态扩缩容、优雅关闭、异常传播 |
| **要求** | 配套写 3 个 benchmark（任务吞吐 vs 线程数 / 任务粒度 / 有无锁竞争） |
| **说明** | 这个项目不为了简历，是为了让你**第一次体会"自己设计并发系统"**。
  写完之后你对 GPU 的 block/warp 调度会天然有感觉 |

### 🔍 M2 月末自检

能否在白板上解释：为什么 cache line 是 64 字节？什么是伪共享？为什么需要内存序？

---

## 阶段二：GPU 编程入门（第 3-4 月）

> **方法**：白天在工作日用本机 UHD 730 写 **SYCL**（概念对照见 [README §5](README.md)），
> 晚上回家用 4070 Ti Super 写 **CUDA**。同一个算法写两遍。

### 第 9 周 · GPU 架构与第一个 kernel

| 项目 | 内容 |
|---|---|
| **主教材** | **PMPP**（《Programming Massively Parallel Processors》第 4 版）第 1-3 章 |
| **CUDA 官方** | 《CUDA C++ Programming Guide》第 1-3 章 |
| **核心概念** | SM（流多处理器）、warp（32 线程）、SIMT、latency hiding、为什么要海量线程 |
| **第一个程序** | `vector_add`：kernel 定义、`cudaMalloc`、`cudaMemcpy`、grid/block 计算、错误检查宏 |
| **本机对照** | `code/01-vector-add-sycl` —— 同样的算法用 SYCL 在核显上跑 |
| **产出** | 两个版本跑通，记录两者的耗时（**注意：核显耗时不能和独显比较**） |

**必做的工程习惯**：写一个 `CUDA_CHECK` 宏包裹所有 API 调用。
90% 的 CUDA 新手 bug 是没检查返回值。

### 第 10 周 · 线程组织与内存访问模式

| 项目 | 内容 |
|---|---|
| **主教材** | PMPP 第 4 章、第 5 章、第 6 章 |
| **核心概念** | grid-stride loop；**memory coalescing（合并访存）**；occupancy 初步 |
| **实验** | 写同一算法的**坏版本 vs 好版本**：① 步长 1 访问 vs 步长 stride 访问
  ② 用 Nsight Compute 看 `gld_efficiency` 指标，亲眼看到 100% vs 12% 的差别 |
| **产出** | 一份对比报告：同一个 kernel，访存模式不同导致 8 倍性能差 |

**这周是转折点**：你会第一次真切感受到"GPU 编程不是写逻辑，是写访存"。

### 第 11 周 · Shared Memory 与分块

| 项目 | 内容 |
|---|---|
| **主教材** | PMPP 第 7 章（卷积）、第 8 章（tiled 矩阵乘） |
| **核心概念** | `__shared__`、`__syncthreads()`、bank conflict、tiling 提高数据复用 |
| **实验** | **矩阵乘法三连**：① naive 版 ② tiled 版 ③ tiled + 寄存器分块版。
  每一步都测 GFLOPs 并截图 |
| **本机对照** | SYCL 的 `local_accessor` + `group_barrier` 写同样的 tiled matmul |
| **产出** | `code/02-matmul` —— 这是你第一个"能写进简历"的东西 |

### 第 12 周 · 性能分析工具

| 项目 | 内容 |
|---|---|
| **工具** | **Nsight Compute (ncu)** 和 **Nsight Systems (nsys)** —— 装好、跑通、理解报告 |
| **核心指标** | `sm__throughput`、`dram__throughput`、`l1tex__throughput`、
  `achieved_occupancy`、`warp_issue_stalled_*` |
| **核心方法** | **Roofline 模型**：判断你的 kernel 是 compute-bound 还是 memory-bound |
| **练习** | 拿上周的 matmul 三连，用 ncu 逐个分析，写出每个版本的瓶颈判断 |
| **产出** | `reports/` 目录，放所有 profiler 截图和你的分析文字 |

**从这周起，你写的每个 kernel 都必须附 profiler 数据。这是铁律二。**

### 第 13 周 · 经典并行模式：归约与扫描

| 项目 | 内容 |
|---|---|
| **主教材** | PMPP 第 10 章（归约）、第 11 章（前缀和） |
| **练习** | **归约五连**：① 交错寻址 ② 相邻寻址 ③ 展开（unroll）④ warp shuffle 版 ⑤ 多 block 两级归约。
  每步测性能，看它如何逼近内存带宽上限 |
| **练习** | 前缀和（prefix sum）：naive → work-efficient (Blelloch) → 单遍扫描 |
| **说明** | 归约和扫描是**面试最高频的手撕题**。这周的内容要练到能闭卷默写 |

### 第 14 周 · 进阶主题

| 项目 | 内容 |
|---|---|
| **主题** | ① 原子操作与 `atomicAdd` ② warp divergence 与其代价 ③ 寄存器压力与 occupancy 权衡
  ④ `__restrict__`、`const`、`#pragma unroll` 的作用 ⑤ 动态并行 |
| **主教材** | 《Professional CUDA C Programming》第 5、6 章；CUDA Guide 的性能指南章节 |
| **练习** | 写一个 histogram kernel：naive（全局原子）→ shared memory 私有化 → 解决 bank conflict，
  看性能如何从 5 GB/s 爬到 200 GB/s |

### 第 15 周 · 流、异步与 CUDA Graph

| 项目 | 内容 |
|---|---|
| **主题** | `cudaStream_t`、pinned memory（`cudaMallocHost`）、`cudaMemcpyAsync`、
  事件计时、重叠计算与传输、**CUDA Graph** |
| **练习** | ① 用多流把一个串行任务流水线化，测加速比 ② 把同样任务用 CUDA Graph 表达，比较 launch 开销 |
| **关键** | 理解 **PCIe 带宽是瓶颈**：host↔device 拷贝有多贵，怎么用重叠把它藏起来 |

### 第 16 周 · 🏆 项目一：自研 CUDA kernel 库 + 基准报告

| 项目 | 内容 |
|---|---|
| **产出** | 一个 GitHub 仓库，包含： |
| | · `vector_add` / `saxpy` / `reduction` / `scan` / `matmul`（多版本）/ `histogram` / `softmax` |
| | · 每个 kernel 配套 benchmark（计时 + 与 CPU / cuBLAS 对比） |
| | · 每个 kernel 配套 Nsight 截图 + 瓶颈分析文字 |
| | · 设计清晰的 CMake 工程 + README（含性能表格） |
| **README 要求** | 首页放一张性能对比表：`kernel | 版本 | 耗时 | GFLOPs / GB/s | 达到峰值的百分比 | 瓶颈` |

**这是你的第一个作品。** 面试官看到这种仓库会立刻知道你是认真的。

### 🔍 M4 月末自检 —— 决策点

闭卷完成以下三项（可以查 CUDA 官方文档，但不能查自己旧代码、不能用 AI）：

1. 手写一个高效的归约 kernel（含 warp shuffle）
2. 手写 tiled 矩阵乘法
3. 指着一个 kernel 说出它是不是 memory-bound，为什么，怎么改

**能通过 → 进入 P3，并按 §分叉 选择门 A / 门 B**
**不能通过 → 用 2 周补齐，P3 顺延**

---

## 阶段三：分叉（第 5-6 月）

> ⚠️ **两条路选一条走**，不要都学。人的精力有限，贪多必失。
>
> **怎么选**：
> - 你享受**调试、剖析系统、和硬件/驱动打交道** → **门 A**
> - 你享受**算法、数学、把性能数字往上推** → **门 B**
> - 只想尽快进 NVIDIA → **门 A**（你的背景是真实资产，别浪费）
> - 数学基础一般（线性代数/数值方法）→ 强烈建议 **门 A**

---

### 🅰️ 门 A 路线：工具 / 驱动 / Profiler

#### 第 17-18 周 · Direct3D 12 基础

| 项目 | 内容 |
|---|---|
| **为什么选 D3D12** | ① 你的背景是 Windows ② NVIDIA Nsight Graphics 在 Windows 上主要抓 D3D ③ JD 明确写 "Direct3D 或 Vulkan 之一" |
| **主教材** | *Introduction to 3D Game Programming with DirectX 12* (Frank Luna) 或微软官方 D3D12 文档 + DirectX-Graphics-Samples |
| **核心概念** | 命令队列/命令列表、描述符堆、资源屏障、根签名、PSO、同步（fence） |
| **练习** | ① 跑通微软官方 HelloTriangle ② 改写成一个旋转的立方体 ③ 加一个 compute shader 做图像处理 |
| **并行** | 同期开始看 **Vulkan**：过一遍 vulkan-tutorial.com，重点是 compute pipeline 和 memory management |

#### 第 19 周 · Windows 系统编程深化

| 项目 | 内容 |
|---|---|
| **主教材** | 《Windows Internals》第 7 版 Part 1：第 1-5 章（架构、进程线程、内存管理） |
| **主题** | 内核对象、句柄表、线程调度优先级、IOCP（完成端口）、ETW、结构化异常处理 (SEH) |
| **练习** | ① 用 IOCP 写一个高性能网络服务器 ② 用 ETW 采集一个进程的 CPU/GPU 活动 |
| **关键** | JD 里的 "system-level programming" 指的就是这些 |

#### 第 20 周 · 图形调试与崩溃分析

| 项目 | 内容 |
|---|---|
| **工具** | **WinDbg**（+ 内核调试）、**RenderDoc**、**PIX**、**Visual Studio 图形调试器** |
| **技能** | ① 分析 minidump：`!analyze -v`、调用栈、寄存器、异常记录 ② 找内存泄漏
  ③ 调试 GPU hang / device removed 错误 ④ 用 Application Verifier 查句柄泄漏 |
| **练习** | 人为制造 5 种崩溃（空指针、栈溢出、堆破坏、死锁、句柄泄漏），逐个用 WinDbg 定位 |

#### 第 21-22 周 · GPU 驱动架构与 WDDM

| 项目 | 内容 |
|---|---|
| **主题** | **WDDM 架构**：UMD（用户态驱动）/ KMD（内核态驱动）的分工、D3DKMT 接口、
  GPU 调度（context/queue）、显存管理（VidMm）、TDR（超时检测与恢复） |
| **资源** | 微软 Learn 的 "Windows Display Driver Model" 章节；ReactOS/WineD3D 源码作为参考实现 |
| **理解层次** | 不需要写驱动，但**要能画出从「应用调用 D3D12 API」到「GPU 执行」的完整链路图** |
| **加分** | 读一读 WDDM 的 GPU 调度器如何做抢占（preemption）—— 这是 profiler 岗位的核心 |

#### 第 23 周 · 🏆 项目二：自研 GPU 监控/分析工具

| 项目 | 内容 |
|---|---|
| **产出** | 一个 Windows 工具，实时采集并可视化：GPU 利用率、显存占用、引擎占用、功耗、温度 |
| **技术** | ETW（Microsoft-Windows-DxgKrnl provider）+ PDH + Performance Counter + D3DKMT API |
| **UI** | 用 Qt 或 Dear ImGui 写界面，要求实时刷新不掉帧（这就是"高性能 UI"） |
| **为什么好** | 这个项目**同时命中了 JD 里的**：Windows 系统编程 + GPU 架构理解 +
  profiling 数据采集 + Qt/GUI —— 几乎是照着 NVIDIA 的 JD 长的 |

#### 第 24 周 · 收尾与整合

- 复习：把前 23 周的笔记整理成一份自己的知识地图
- 补短板：哪块最虚补哪块
- 开始准备简历和项目讲解稿

---

### 🅱️ 门 B 路线：GPU 计算 / 算子优化

#### 第 17-18 周 · Tensor Core 与 CUTLASS

| 项目 | 内容 |
|---|---|
| **主题** | 第 4 代 Tensor Core（Ada）、MMA 指令、`wmma` API、`mma.sync` PTX、
  数据类型（fp16 / bf16 / tf32 / fp8）与混合精度 |
| **主教材** | CUDA Guide 的 "Warp Matrix Functions" 章节；CUTLASS 官方文档与 examples |
| **练习** | ① 用 `wmma` API 手写一个 fp16 矩阵乘法 ② 用 `mma.sync` PTX 内联汇编写一个 m16n8k16 的 MMA |
| **CUTLASS** | 跑通 CUTLASS 的 example，读懂它的 GEMM 分层结构（threadblock/warp/thread level） |
| **注意** | ⚠️ 这一块**核显学不了**，必须用 4070 Ti Super。SYCL 的 joint_matrix 概念类似但生态差很远 |

#### 第 19-20 周 · 手写高性能 GEMM

| 项目 | 内容 |
|---|---|
| **目标** | 从零手写一个 GEMM，性能达到 cuBLAS 的 70% 以上（在 4070 Ti Super 上） |
| **优化阶梯** | ① naive ② tiled shared memory ③ 寄存器分块（每线程算 8×8）
  ④ 双缓冲 + 异步拷贝（`cp.async`）⑤ 向量化访存（`float4`）⑥ Tensor Core 版 |
| **工具** | 每步都用 `ncu` 分析，关注 compute/memory throughput、occupancy、stall reason |
| **产出** | 每一步的性能数据表 —— **这张表本身就是极强的简历素材** |

**这 2 周会很痛苦。** GEMM 优化是 GPU 计算的"九九乘法表"，
过去了就是一片新天地，过不去就永远停留在"会写 CUDA"的层次。

#### 第 21-22 周 · 复现 FlashAttention

| 项目 | 内容 |
|---|---|
| **论文** | 《FlashAttention》和《FlashAttention-2》（精读，包括附录的推导） |
| **核心思想** | IO-aware 算法：用 online softmax 避免物化 N×N 注意力矩阵，
  用 tiling 让数据留在 SRAM |
| **练习** | ① 先写 naive attention（物化矩阵版）② 再写 flash attention 前向 ③ 加 causal mask
  ④ 加反向传播（可选，很难）|
| **对比** | 与 PyTorch 的 `F.scaled_dot_product_attention` 对比性能和显存占用 |
| **为什么** | FlashAttention 是当前 AI 系统岗**最常被问的项目**。能讲清楚 online softmax 的推导，
  面试通过率大幅提升 |

#### 第 23 周 · Triton 与图优化

| 项目 | 内容 |
|---|---|
| **Triton** | OpenAI Triton 入门：用 Python DSL 写 kernel，理解它的 block-level 抽象 |
| **练习** | 用 Triton 重写你的 softmax / layernorm / FlashAttention，与手写 CUDA 版对比 |
| **图优化** | 算子融合（kernel fusion）、CUDA Graph 在推理中的应用、内存池 |
| **量化** | INT8 / FP8 量化基础，写一个量化的 GEMM kernel |

#### 第 24 周 · 🏆 项目二：PyTorch 自定义算子库

| 项目 | 内容 |
|---|---|
| **产出** | 一个 pip 可安装的 PyTorch 扩展包，包含你写的高性能算子 |
| **技术** | `torch.utils.cpp_extension` + `setup.py` + pybind11 + CUDA kernel |
| **算子** | fused softmax、layernorm、FlashAttention、量化 GEMM |
| **要求** | 每个算子：正确性测试（对拍 PyTorch）+ 性能测试 + 与官方实现的对比表 |
| **为什么好** | 这个项目**直接对应"AI Infra 算子工程师"的日常**，
  且展示了 PyTorch 生态能力（门 B 面试必问） |

---

## 阶段四：作品集与硬功（第 7-9 月）

### 第 25-28 周（M7）· 项目三 + LeetCode 启动

| 项目 | 内容 |
|---|---|
| **项目三** | 选一个**有原创性**的题。建议方向：
  · 用 CUDA 加速一个真实算法（图算法 / 稀疏矩阵 / 分子动力学 / 光线追踪）
  · 实现一个论文里的 kernel（如 FlashDecoding、PagedAttention、Ring Attention）
  · 给你司的某个真实计算任务做 GPU 加速（**这个最好，因为只有你能讲**） |
| **LeetCode** | 每周 10 题，专注 C++，重点是数组/字符串/链表/树/DP/并发 |
| **要求** | 每道题**先自己写，卡住 30 分钟再看题解，看懂后关掉重写** |

### 第 29-32 周（M8）· 开源贡献

⚠️ **这是整条路线里性价比最高的一步。**

| 项目 | 内容 |
|---|---|
| **目标** | 在 NVIDIA 相关的开源项目里**合入至少 2 个 PR** |
| **候选项目** | · **CUTLASS**（NVIDIA 官方，门 B 首选）
  · **Nsight 相关工具/示例**（门 A）
  · **Triton** / **PyTorch** / **flash-attention**（AI Infra 通吃）
  · **VTK / Qt**（门 A 相关） |
| **入门路径** | 从 `good first issue` 标签开始 → 文档修正 → 测试补充 → 小 bug 修复 → 性能优化 |
| **为什么有用** | ① NVIDIA 的工程师真的会看这些 PR ② 面试时"我给 CUTLASS 提过 PR"
  比"我学过 CUDA"强 100 倍 ③ 你会学到工业级代码规范 |
| **额外产出** | 同期开始写技术博客（掘金/知乎/GitHub Pages），把学习过程沉淀成 5-10 篇 |

### 第 33-36 周（M9）· 面试硬功

| 项目 | 内容 |
|---|---|
| **C++ 八股** | 虚函数表、内存布局、智能指针实现、模板特化、STL 源码级理解 |
| **体系结构** | cache 一致性、NUMA、SIMD、分支预测、GPU vs CPU 架构对比 |
| **操作系统** | 进程调度、虚拟内存、文件系统、锁的实现、Linux 系统调用（门 A 重点） |
| **手撕题** | 归约、扫描、矩阵乘、softmax、直方图 —— **闭卷、限时、白板级** |
| **系统设计** | 设计一个 profiling 工具 / 设计一个推理框架的调度器 |
| **英语** | NVIDIA 面试有英文环节。每天 30 分钟：技术英文播客 + 用英文讲一遍你的项目 |
| **简历** | 一页，项目占 60%，每个项目配性能数字 |

---

## 阶段五：投递与面试（第 10-12 月）

### 第 37-40 周（M10）· 练手面试

**策略：先拿不心动的 offer 练手，再面想去的。**

| 顺序 | 目标 | 目的 |
|---|---|---|
| 1 | **国产 GPU 厂商**：摩尔线程、壁仞、沐曦、天数智芯、燧原、砺算 | 真实面试体感、检验知识盲区 |
| 2 | **AI Infra**：字节 AML、阿里 PAI、腾讯、商汤、以及大模型创业公司 | 薪资可能超过 NVIDIA，且门槛更友好 |
| 3 | **自动驾驶 / 芯片**：地平线、黑芝麻、蔚来、小鹏 | 同类岗位，多一个选择 |
| 4 | **NVIDIA**（门 A 岗位优先） | 目标 |

**每次面完立刻复盘**：被问倒的问题写进 `interview-log.md`，一周内补上。

### 第 41-48 周（M11-M12）· 冲刺与谈判

- 集中投 NVIDIA 上海的门 A 岗位（GPU Driver Profiler / Graphics Tools / Profiling）
- 内推优先：LinkedIn 找 NVIDIA 上海员工、技术社区、前同事
- 用 M10 拿到的 offer 作为谈判筹码
- **不要裸辞**，拿到书面 offer 再走

---

## 6 个月加速版（仅当每周投入 ≥ 25 小时）

**只走门 A。** 压缩方案：

| 原计划 | 压缩为 | 怎么压 |
|---|---|---|
| M1-M2 C++/系统 | **6 周** | 砍掉 Malloc Lab、Cache Lab；只保留 CSAPP 第 3/6/7/9 章 + 手写容器 |
| M3-M4 CUDA | **6 周** | 砍掉 scan、CUDA Graph、histogram；保留 vector_add/matmul/reduction 三件套 + profiler |
| M5-M6 门 A | **8 周** | 只学 D3D12（跳过 Vulkan）+ WinDbg + WDDM 概念 + 项目二 |
| M7-M9 作品集 | **4 周** | 项目从 3 个减到 2 个；开源贡献照做（不能省） |
| M10-M12 投递 | **2 周** | 直接投，边面边补 |

**加速版的代价**：CUDA 深度不足，只能投门 A，且面试中被问到 CUDA 细节会吃亏。
**不推荐，除非你有明确的年龄/经济压力。**

---

## 每周节奏模板

| 时段 | 内容 |
|---|---|
| 工作日通勤 | 听技术播客 / 看论文（不写代码） |
| 工作日午休 40min | 本机写 SYCL 练习 / 刷 LeetCode |
| 工作日晚 2h | **主战场**：家里 CUDA 开发 + profiling |
| 周六 4h | 项目推进 |
| 周日 2h | 复盘 + 写笔记/博客 + 交接下周任务 |
| 周日 1h | **周自检**（见下） |

## 每周自检三问

1. **本周我写的最难的一段代码，能脱离 AI 从空白重写吗？**
2. **本周产出的代码，有对应的性能数字 / profiler 数据吗？**
3. **如果面试官现在问我这周学的东西，我能讲 10 分钟吗？**

三个都是"是" → 继续。有一个"否" → 下周补。

---

## 参考书单（按优先级）

| 优先级 | 书 | 用途 | 阶段 |
|---|---|---|---|
| ⭐⭐⭐ | **CSAPP**《深入理解计算机系统》3e | 系统地基，门 A 面试核心 | P1 |
| ⭐⭐⭐ | **PMPP**《Programming Massively Parallel Processors》4e | CUDA 圣经 | P2-P3 |
| ⭐⭐⭐ | **CUDA C++ Programming Guide**（官方，免费） | 权威参考 | P2-P3 |
| ⭐⭐⭐ | 《Effective Modern C++》 | C++ 去水分 | P1 |
| ⭐⭐ | 《Professional CUDA C Programming》 | CUDA 进阶与性能 | P2 |
| ⭐⭐ | 《C++ Concurrency in Action》 | 并发 | P1 |
| ⭐⭐ | 《Windows Internals》7e Part 1 | 门 A 核心 | P3(A) |
| ⭐⭐ | CUTLASS 官方文档 + examples | 门 B 核心 | P3(B) |
| ⭐ | Frank Luna *DX12* 或 vulkan-tutorial.com | 门 A 图形 API | P3(A) |
| ⭐ | 《Computer Architecture: A Quantitative Approach》 | 体系结构进阶 | P4 |

**免费在线资源**
- NVIDIA DLI 课程（部分免费，含云端 GPU 实验环境）
- NVIDIA LaunchPad（免费 8 小时 GPU 实验）
- CUDA 官方 Samples（`cuda-samples` GitHub 仓库）
- Intel oneAPI 官方 SYCL 教程 + SYCL↔CUDA 移植指南
