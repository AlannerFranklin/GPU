# GPU 学习路线 · 总纲

> 创建日期：2026-10-08
> 目标：12 个月内拿到 NVIDIA（或同级）年薪 40w+ 的 offer
> 起点：C++ 基础薄弱，现有项目依赖 AI 生成；在职 Windows 上位机软件工程师

---

## 0. 先说结论（不好听但重要）

**你的原始设想——「自学 CUDA → 半年跳 NVIDIA 做 GPU 计算」——成功率低于 10%。**

原因不是你不努力，而是这个岗位的供给结构：

- NVIDIA 的 CUDA / DevTech / 算子优化岗，招的是**已经能独立优化 kernel 的人**。
  这个能力通常来自 3-5 年的实际工作积累，不是 6-12 个月自学能速成的。
- 你的竞争对手是：清北复交+CMU 的体系结构硕博、超算中心出来的 HPC 工程师、
  大厂 AI Infra 团队里天天和 kernel 打交道的人。他们在 **起跑线上就已经在跑**。

**但是**，我查了 NVIDIA 上海正在招的岗位，发现了一条几乎为你背景量身定做的路径：

> NVIDIA 上海在招 **GPU Driver Profiler Engineer**，明确要求 **Windows WDDM** 驱动经验；
> 以及 **Graphics Tools Software Engineer**（Nsight 工具团队），要求 C++、系统级编程、
> Qt/GUI 经验、崩溃转储分析。

你现在的本职——**给硬件写 Windows 软件操作界面**——和这两个岗位的匹配度，
比你想的高得多。这是一条**侧门**。

所以本路线图是**双轨制**：

| | 门 A：侧门（工具/驱动圈） | 门 B：正门（GPU 计算/算子圈） |
|---|---|---|
| 目标岗位 | GPU Driver Profiler / Graphics Tools / Nsight | DevTech / Compute Architecture / 算子优化 |
| 你的匹配度 | **高**（Windows + 硬件 + UI 直接对口） | 低（需要从零建立） |
| 12 个月能达成的概率 | **30-50%** | 10-15% |
| 薪资 | 40-60w（你的经验可谈） | 40-70w（但门槛高） |
| 面试考什么 | C++、操作系统、调试、图形 API、性能分析 | CUDA kernel 手写、体系结构、并行算法 |
| 学的东西 | 通用系统能力，不押注单一技术 | 深度专精，方向窄但深 |

**我的建议：A 为主线，B 为副线。**

两条线在前 4 个月学的东西**完全重合**（C++ + 计算机体系结构 + GPU 基础），
你在第 4 个月末再决定主攻哪个，不浪费任何时间。而且 A 线学的东西（系统级 C++、
调试、性能分析）在 B 线面试里也是加分项，反之不然。

---

## 1. 现状盘点

### 1.1 本机（工作日白天 / 午休 / 通勤）

| 项目 | 配置 | 评价 |
|---|---|---|
| CPU | Intel i3-14100 (4C/8T) | 够用。CPU 侧练习、跑 OpenMP 都没问题 |
| GPU | Intel UHD Graphics 730 (24 EU, Gen12 Xe-LP) | **不是废铁**，支持 SYCL / OpenCL 3.0 / Level Zero |
| 内存 | **8 GB** | ⚠️ **最大瓶颈**，见 §4 |
| 磁盘 | 331 GB 可用 | 充足 |
| 已装 | VS Community 2026 (MSVC 14.51 + CMake + Ninja)、Python 3.14.6、Git、VS Code | 工具链基本齐了，只是没配 PATH |

**关键认知**：UHD 730 虽然是核显，但它是**真正的可编程 GPU**。
你可以在这台机器上写 SYCL 和 OpenCL 并真的跑在 GPU 上——
SIMT 执行模型、work-group、local memory、barrier、矩阵分块……这些概念
**和 CUDA 是 1:1 对应的**（见 §5 对照表）。白天用核显学概念，
晚上回家用 4070 Ti Super 验证 CUDA，效率远高于"白天只看书"。

### 1.2 家里（晚间 / 周末）

| 项目 | 配置 | 评价 |
|---|---|---|
| GPU | **RTX 4070 Ti Super 16GB** | 👍 **非常好的学习卡**，见下 |

- 架构 Ada Lovelace (AD103)，**Compute Capability 8.9**
- 8448 CUDA Core / 66 SM / 第 4 代 Tensor Core (264 个)
- 16GB GDDR6X —— 够跑真实的 LLM 推理、大矩阵、完整 FlashAttention 实验
- 支持 bf16 / tf32 / **FP8**（Transformer Engine 那套），支持 DP4A

**结论：硬件条件完全够用，不需要买卡。** 这张卡的唯一短板是
只有单卡、显存 16GB（做多卡通信 / 超大模型实验需要租云）。
AutoDL / 恒源云租 A100/H100 大约 2-10 元/小时，需要时再租，不用常备。

### 1.3 技能起点（这才是真正的短板）

- C++：有基础，但**未经受过高强度检验**
- 项目经历：GitHub 上有仓库，但**大部分是 AI 生成的**
- 工程能力：Windows 桌面软件开发、和硬件打交道 → **这是资产，不是负债**

⚠️ **「AI 帮我写的」这件事，是整个计划里最大的风险。**

不是因为丢人，而是因为：**面试是白板的，没有 AI。**
面试官会让你现场写一个 reduction kernel、追问为什么用 shared memory、
让你解释一个 race condition。AI 给你的代码你如果没真正理解，
在那些追问面前会在 30 秒内暴露。

所以本路线图有一条贯穿始终的铁律，写在 §6。

---

## 2. 两条路线的岗位要求（真实招聘信息）

以下是 NVIDIA 上海正在招的岗位，我按你的匹配度排序。

### 🟢 门 A 岗位

**A1. GPU Driver Profiler Engineer（上海）** ← 匹配度最高

- 职责：开发维护支持 NVIDIA 性能分析工具的 GPU 内核/固件模块，
  支持 **Windows WDDM**、Linux、QNX 等多种驱动架构
- 硬性要求：BS + 2 年经验；优秀 C/C++；**愿意调试 UMD/KMD（用户态/内核态驱动）交互**
- 加分项：CPU/GPU 硬件架构知识；内核中的功耗/性能/时钟控制；
  CUDA/OpenCL/OpenGL/DirectX 任一；嵌入式 Linux/RTOS
- **为什么适合你**：Windows + 硬件 + C++ 三个关键词你都沾边。
  "调试硬件和软件的交界处"这件事你天天在做，只是层次浅。

**A2. Graphics Tools Software Engineer（上海，Nsight 团队）**

- 职责：为 Nsight 图形工具套件实现新功能，高性能 C++，系统级/图形调试
- 硬性要求：优秀 C++ 与面向对象设计；**精通 Direct3D 或 Vulkan 之一**；
  扎实系统级编程（Linux 进程/线程/IPC/内存管理/socket/系统调用）；
  熟练 GDB/LLDB/Valgrind/strace；**能分析 crash dump 和 core file**；3 年经验
- 加分项：3D 图形算法与 GPU 架构；异构计算与多线程（SM、warp 调度）；
  GPU 性能调优与 profiling；**Qt 框架与 GUI 开发经验**
- **为什么适合你**：如果你在做上位机 UI，大概率用过 Qt 或 MFC/WPF。
  **Qt 是明确写出来的加分项。** 缺口是 D3D12/Vulkan + Linux 系统编程。

**A3. GPU Profiling Software Engineer（上海）**

- 硬性要求：C/C++/Python 扎实；**深入理解计算机体系结构（x86/ARM/GPU）和操作系统**；
  数据结构与算法；EE/CS 本科 + 4 年 或 硕士 + 2 年
- 加分项：设备驱动或系统软件开发；CUDA/OpenCL/OpenGL/D3D/Vulkan；
  GPU 开发者工具经验；**能读汇编**
- **为什么适合你**：门槛偏学术，但"计算机体系结构 + 操作系统"这两门
  是可以靠 CSAPP + 刷题在 8 个月内补到面试水平的。

### 🔴 门 B 岗位

**B1. Developer Technology Engineer – AI（上海/北京/深圳）**

- 要求：精通 C/C++ 与 Python；**加速计算经验，最好掌握 CUDA**；
  能编写并优化 kernel；并行编程；线性代数/数值方法；2-3 年经验
- **注意**：部分 DevTech 岗位 JD 里明确写了
  *"不要求已掌握全部 GPU/CUDA 知识，优秀的 C++ 工程师也欢迎投递"*
  —— 这是门 B 唯一的裂缝，但"优秀的 C++ 工程师"这个定语很重。

**B2. Compute Architecture Software Engineer（上海，Staff+）**
- 5+ 年经验，这是你 5 年后的目标，现在不用看

### 薪资参考

| 来源 | 数字 |
|---|---|
| 猎聘·NVIDIA 中国 深度学习算法工程师 | 3.5-6.5 万/月（42-78w/年） |
| Levels.fyi 估算·DevTech AI 上海 | 约 S$7,500-12,625/月 |
| 猎聘·GPGPU 用户态驱动工程师（同类岗位，非 NVIDIA） | 50-70k × 14 薪 |

**你的 40w 目标**：通过门 A 进去，以你现有的 Windows + 硬件经验，
**是合理目标**。NVIDIA 官方 JD 不公开数字，只写 "highly competitive"。

---

## 3. 12 个月地图

```
月份   1    2    3    4    5    6    7    8    9    10   11   12
       ├──────────┼──────────┼──────────┼──────────┼──────────┤
阶段   │  P1 地基  │  P2 GPU  │  P3 分叉  │  P4 作品集 │  P5 面试  │
       │ C++/系统  │ 编程入门  │  选轨道   │  与硬功    │  与投递   │
       ├──────────┼──────────┼──────────┼──────────┼──────────┤
门A    │          │ 共同基础  │ D3D12/   │ Nsight 类 │ 投递      │
       │          │          │ Vulkan/  │ 工具复刻   │ NVIDIA    │
       │          │          │ WDDM     │           │ 上海      │
       ├──────────┼──────────┼──────────┼──────────┼──────────┤
门B    │          │ 共同基础  │ CUTLASS/ │ 手写       │ 投递      │
       │          │          │ FlashAttn │ 算子库     │ 国产GPU   │
       ├──────────┼──────────┼──────────┼──────────┼──────────┤
里程碑 │ 手写     │ CUDA     │ 确定     │ 3 个可讲   │ offer     │
       │ 500行C++ │ 基础通关  │ 主攻方向  │ 30min项目  │           │
```

**6 个月加速版**：只有在每周稳定投入 **25 小时以上** 时才可行，
且只能走门 A。压缩方式见 `ROADMAP.md` 末尾。

详细周计划见 **[ROADMAP.md](ROADMAP.md)**，环境搭建见 **[SETUP.md](SETUP.md)**。

---

## 4. 硬件与工具方案

| 场景 | 用什么 | 干什么 |
|---|---|---|
| 工作日白天/午休 | 本机 i3-14100 + **UHD 730** | 写 SYCL/OpenCL，**真的跑在 GPU 上**；读文档；刷 LeetCode |
| 工作日晚间 | 家里 **4070 Ti Super 16G** | CUDA 主力开发、profiling、跑实验 |
| 周末 | 家里 + 按需租云 | 大显存实验、多卡、复现论文 |
| 需要大显存/多卡 | AutoDL / 恒源云 | A100 约 5-10 元/小时，用完即停 |

**💰 唯一建议花钱的地方：把本机内存从 8GB 加到 32GB。**

- 一条 16GB DDR5/DDR4 大约 200-400 元（取决于你主板是 DDR4 还是 DDR5）
- 为什么：8GB 下你连 VS + 浏览器 + 一个模板稍多的 C++ 项目都跑不动，
  编译会频繁触发 swap，直接把学习体验毁掉。**这是回报率最高的一笔投入，
  优先级高于任何课程。**

**不需要买显卡。** 4070 Ti Super 够你学到很深。

---

## 5. 核显学习法：SYCL ↔ CUDA 概念对照

这是本路线图的核心技巧。你白天在核显上写 SYCL，
晚上回家把同样的概念用 CUDA 重写，**学一遍顶两遍**。

| CUDA | SYCL (oneAPI) | 说明 |
|---|---|---|
| `__global__` kernel | `queue.parallel_for` | 提交并行任务 |
| `threadIdx.x` | `item.get_local_id(0)` | 组内线程 ID |
| `blockIdx.x` | `item.get_group(0)` | 工作组 ID |
| `blockDim.x` | `item.get_local_range(0)` | 组内线程数 |
| `gridDim.x` | `item.get_group_range(0)` | 工作组数 |
| `__shared__` | `local_accessor` | 片上共享内存 |
| `__syncthreads()` | `group_barrier(group)` | 组内同步 |
| warp (32 线程) | sub-group（Intel 上通常 8/16/32） | SIMT 执行单元 |
| `cudaMalloc` / `cudaMemcpy` | `malloc_device` / `queue.memcpy` | 设备内存与拷贝 |
| `cudaStream_t` | `queue` | 执行队列 |
| Nsight Compute | Intel VTune / Advisor | 性能分析 |
| `nvcc` | `icpx -fsycl` | 编译器 |

**Intel 官方甚至提供了一份 SYCL ↔ CUDA 移植指南**，
说明这两者本来就是同一套心智模型。

⚠️ 差异（要在心里记牢，别搞混）：
- Intel 核显没有 Tensor Core（没有 XMX 矩阵引擎），
  所以 MMA / wmma / CUTLASS 这部分**只能回家用 4070 学**。
- sub-group 宽度不固定，而 CUDA warp 恒为 32。
- 核显和 CPU 共享内存带宽，性能模型和独显不同 ——
  **不要用核显的性能数据去推断独显**，只用它验证正确性。

---

## 6. 三条铁律

这三条比任何技术细节都重要。违反其中任何一条，12 个月后你会发现自己
GitHub 上多了 10 个仓库，但面试时一个问题都答不上来。

### 铁律一：AI 只能当"老师"，不能当"手"

**允许**：让 AI 解释概念、review 你的代码、指出 bug、生成测试数据、
解释报错信息。**这相当于请了个 24 小时家教。**

**禁止**：
- 让 AI 写出核心逻辑然后你复制粘贴
- 看不懂的代码留在你的仓库里
- 用 AI 生成的项目当作自己的作品集

**自检方法**：每周挑一个你本周写的函数，**关掉所有 AI 和文档，
从空白文件重写一遍**。写不出来 = 这周白学了。

### 铁律二：每个 kernel 都要有性能数字

从第一个 CUDA 程序开始，就养成习惯：
**代码 + 计时 + profiler 截图**，三者缺一不可。

一个"我写了个矩阵乘法"的仓库没人看；
一个"我把矩阵乘法从 12 GFLOPs 优化到 4.2 TFLOPs，
每一步的 Nsight 截图和瓶颈分析都在这里"的仓库，
面试官会主动问你细节。

**性能数字是 GPU 工程师的简历。**

### 铁律三：项目必须能讲 30 分钟

面试里 90% 的时间花在深挖你简历上的项目。判断标准：

> 随便指着你项目里的任意一行代码，你能说出：
> 为什么这么写？不这么写会怎样？试过什么别的方案？瓶颈在哪？数据是多少？

做不到，这个项目就不该写在简历上。

---

## 7. 今天就能做的三件事

1. **打开 [SETUP.md](SETUP.md)**，把本机工具链配通（约 40 分钟）
2. **跑通 `code/00-hello-sycl`**，亲眼看到代码跑在 UHD 730 上（约 20 分钟）
3. **在心里回答这个问题**：过去半年，我有没有哪段代码是完全可以脱离 AI 写出来的？
   如果答案是"没有"，那你 P1 阶段的任务就是制造出第一个这样的代码。

---

## 参考来源

- [NVIDIA GPU Profiling Software Engineer (Shanghai)](https://nvidia.wd5.myworkdayjobs.com/NVIDIAExternalCareerSite/job/China-Shanghai/GPU-Profiling-Software-Engineer_JR2016747)
- [NVIDIA GPU Driver Profiler Engineer](https://jobs.anitab.org/companies/nvidia/jobs/57710778-gpu-driver-profiler-engineer)
- [NVIDIA Graphics Tools Software Engineer (Shanghai)](https://jobs.anitab.org/companies/nvidia/jobs/83137015-graphics-tools-software-engineer)
- [NVIDIA Developer Technology Engineer – AI (Shanghai)](https://job.careers/job/290cd5fc61721910807d828de9b0354e/)
- [NVIDIA Compute Architecture Software Engineer 岗位分析](https://refresh.cv/jobs/nvidia/fffa83a9-1075-4759-87fa-2955677277c0)
