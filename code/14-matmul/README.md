# 14-matmul · 矩阵乘法优化三阶梯

> ROADMAP 第 11-12 周。硬件：RTX 4070 Ti Super（Ada, CC 8.9，66 SM）+ CUDA 12.6。
> 目标不是「跑通一个 matmul」，是**每一版的性能数字和瓶颈都说得出为什么**。
> 设计推导见 [notes.md](notes.md)；每日过程见 [handover/](../../handover/)。

`C = A × B`，A / B / C 均为 N×N 的 `float` 方阵。

## 0. 文件结构

```
matmul_common.h     分配 / 填数 / 拷贝 / 计时 / 对拍 / 释放 —— 四个版本共用
matmul.cu           naive kernel        + 4 行 main
matmul_tile.cu      shared memory 版     + 4 行 main
matmul_reg.cu       4×4 register 版      + 4 行 main
matmul_reg8.cu      8×8 register 版      + 4 行 main
```

四个 `main` 只写一件事：把自己的 kernel 和配套的 block/grid 交给
`run_benchmark(void (*kernel)(float*, float*, float*, int), dim3 block, dim3 grid, int N)`。

**block/grid 必须跟着 kernel 一起传**，因为它们和 kernel 的下标公式是绑定的——
`32` / `128` / `64` / `256` 这几个常数同时出现在「shared 数组形状」「块内行/列偏移」
「grid 步进」三处，改一个必须改三个。（详见 §2.3 的坐标映射。）

## 1. 性能总表

| 版本 | 每线程输出 | block | shared tile | N=1024 min_ms | N=1024 GFLOPS | N=4096 min_ms | N=4096 GFLOPS |
|---|---|---|---|---|---|---|---|
| `matmul`（naive） | 1 | (32,32) | 无 | 0.817152 | 2628 | 58.793983 | 2338 |
| `matmul_tile` | 1 | (32,32) | 32×32 | 0.635776 | 3377 | 45.563904 | 3016 |
| `matmul_reg` | 4×4 | (32,8) | 32×128 | 0.169984 | 12,630 | 10.061824 | 13,660 |
| `matmul_reg8` | 8×8 | (32,8) | 64×256 | 未测 | 未测 | 未测 | 未测 |

加速比：tiled 对 naive **1.29x**（N=1024）、**1.16x**（N=4096）；reg 对 tiled **3.74x**（N=1024）、**4.53x**（N=4096）。

> 计时口径：`cudaEvent`，预热 1 次 + 计时 3 次取最小值；`cudaMemcpy` 一律不进计时窗口。
> 前三行数据来自 **2026-11-14 晚同一次运行**（同一轮实验横向比才公平；与 11-09/10 的
> 单独测量差 3-7%，在抖动范围内）。
> ncu（管理员终端）实测：`local_ld = 0` ✅、`dram__bytes.sum = 4.35 GB`（N=4096，reg 版）。

### 资源用量（`-Xptxas -v` 实测，CUDA 13.4，仅供参考）

| kernel | 寄存器/线程 | shared/block | spill | 每 SM 可开 block | occupancy |
|---|---|---|---|---|---|
| `matmul` | 40 | 0 | 0 | 1（1024 线程卡死） | 66.7% |
| `matmul_tile` | 36 | 8 KB | 0 | 1（同上） | 66.7% |
| `matmul_reg` | 48 | 20 KB | 0 | 5 | 83.3% |
| `matmul_reg8` | 128 | 40 KB | 0 | 2 | 33.3% |

> `spill = 0` 说明 `acc/a/b` 全部落在寄存器里，与 `local_ld = 0` 相互印证。
> **occupancy 不直接决定性能**：naive 66.7% 却比 8×8 慢 5.8 倍。

## 2. 三个版本的设计

### 2.1 naive —— 一个线程算一个输出

```cpp
int row = blockIdx.y * blockDim.y + threadIdx.y;   // .y 管行
int col = blockIdx.x * blockDim.x + threadIdx.x;   // .x 管列
for (int k = 0; k < N; k++)
    sum += A[row * N + k] * B[k * N + col];
```

- `block(32, 32)`、`grid(N/32, N/32)`
- 每线程 2N 次 global load，总计读 **2N³** 次；**每 1 次乘加要 2 次 load 养活**

### 2.2 tiled —— 先用 shared memory 把重复读接住

```cpp
__shared__ float As[32][32];
__shared__ float Bs[32][32];
```

- 形状不变：`block(32,32)`、`grid(N/32, N/32)`、每线程仍是 1 个输出
- 每个 block 只从 global 读 `A` 的一个 32×32 子块和 `B` 的一个 32×32 子块，
  块内 1024 个线程共享 → global 读量降到 **2N³/32**
- 两个 `__syncthreads()`：填完 shared 之后那个防 **RAW**，本轮读完、下一轮写之前那个防 **WAR**。
  删任一个都不会崩，只是数不对
- **不需要 padding**：`As[k][ty]` 同 warp 内地址相同 → 广播；`Bs[k][tx]` 同 warp 内
  `bank = tx`，铺满 0-31 → 无冲突。这与 13-transpose 的 32×33 是两种不同的模式，别混用

### 2.3 register —— 每线程 4×4 个累加器

```cpp
__shared__ float As[32][32];     // A 的 32 行 × 32 个 k
__shared__ float Bs[32][128];    // B 的 32 个 k × 128 列

float acc[4][4] = {0};           // 16 个输出 / 线程
float a[4];
float b[4];
```

- `block(32, 8)` = 256 线程，`grid(N/128, N/32)`，每 block 出 `32 × 128` 个输出
- 内层每个 `k`：**8 次 shared load 养 16 次 FMA**（tiled 是 2 次 load 养 1 次 FMA）
- 三层坐标映射，`y 管行，x 管列` 对 `blockIdx` 和 `threadIdx` 都成立：

```
C 的行 = blockIdx.y × 32 + threadIdx.y × 4 + i
C 的列 = blockIdx.x × 128 + threadIdx.x + j × 32
```

- 生成这条映射的总规则：**块内号 = 连续的那个 + 另一个 × 连续那个的总数**
  （行：`i + ty×4`；列：`tx + j×32`）；反推时**余数永远给「连续的那个」**
- 行为什么嵌套、列为什么交错：列用 `tx + j*32` 而不是 `tx*4 + j`，是为了让一个 warp 的
  32 个线程落在 32 个不同 bank 上；连续写法会 4 路冲突
- 资源（ptxas 实测，不是估算）：**48 reg/thread** → 256×48 = 12,288 reg/block →
  `65536/12288 = 5`；线程上限 `1536/256 = 6`；shared 上限 `228/20 = 11` →
  5 block/SM → **occupancy 83.3%**
  （最初手算估的是 34 reg → 6 block → 100%，**估错了**。寄存器数目以 ptxas 为准。）

### 2.4 8×8 register tiling

同一套公式，TM = TN = 8：

```
行覆盖 = blockDim.y × TM = 8 × 8 = 64      As = [64][32]
列覆盖 = blockDim.x × TN = 32 × 8 = 256    Bs = [32][256]
grid  = (N/256, N/64)
```

- 每线程 64 个累加器 + `a[8]` + `b[8]` = 80 个 float；ptxas 实际给到 **128 个寄存器**，`spill = 0`
- 每 FMA 的 load 从 0.5 降到 **0.25**（16 load 养 64 FMA）
- occupancy 从 83% 掉到 **33%**（2 block/SM）
- shared 40 KB/block（静态 `__shared__` 的 block 上限是 48 KB，卡在这条线下面）
- **两个 y 偏移不一样**：`As` 用 `threadIdx.y * 8`，`Bs` 用 `threadIdx.y * 4`。
  4×4 那版一个 `thread_row = ty*4` 两处通用，这里不行
- 选 `block(32,8)` 而不是 `(16,16)` / `(8,32)`：warp 内 `threadIdx.y` 的取值个数
  = `32 ÷ blockDim.x`，(32,8) 只有 1 种 → 读 `As[r][k]` 全同址 → **广播，1 笔事务**；
  (16,16) 是 2 种 → 2 址挤同一 bank → 2 路冲突；(8,32) 是 4 路

## 3. 两条最重要的结论

**① tiled 只有 1.29x，是因为 L2 替 naive 挡了刀。**
教科书账：naive 读 2N³ = 550 GB，按 672 GB/s 得 800 ms。实测 57 ms。
原因是 A+B 共 8 MB 全在 48 MB 的 L2 里，加上 block 行优先调度，
**naive 的重复读本来就没走 DRAM**。tiled 省的是 global 地址访问，但那本来就命中 L2。

**② 真正的墙是「每 FMA 配多少条 load 指令」，不是「数据在哪」。**
四个组合的 GFLOPS 都卡在 2.4-3.2 TFLOPS，与 N 无关。
这正是 register tiling 的动机，也是它的作用边界：

```
Roofline 算的 AI       = FLOP ÷ 【走 DRAM 的字节】      ← register tiling 改不动这个
register tiling 改善的  = 乘加次数 ÷ 【load 指令条数】    ← Roofline 不管这一层
```

**Roofline 基线**：峰值 FP32 = 66 SM × 128 × 2 × 2.61 GHz = **44.1 TFLOPS**；
带宽 **672 GB/s**；ridge point = 44.1e12 ÷ 672e9 = **65.6 FLOP/byte**。
AI < 65.6 → memory-bound。AI 的字节数**必须取 ncu 的 DRAM 计数器实测值**，
不能按「代码读了几次 × 4」纸上算——那样 naive 会被估成 0.25，和实测差 14 倍。

## 4. 编译与运行

```bat
build.bat 14-matmul run 1024
build.bat 14-matmul run 4096
```

`run` 会把 `build\` 下**四个 exe 依次跑一遍**（naive → tile → reg → reg8）。
只想跑其中一版：

```bat
.\build\matmul_reg8.exe 4096
```

N 从命令行读，缺省 1024。**N 必须是 256 的倍数**（`matmul_reg8` 的 grid 按 /256、/64 切；
另外三版的要求更松，但统一按 256 取整最省事）。

想看每个 kernel 用了多少寄存器和 shared，把 `-Xptxas -v` 留在
`CMakeLists.txt` 的 `CMAKE_CUDA_FLAGS` 里，编译时会打印：

```
ptxas info : Used 128 registers, used 1 barriers, 40960 bytes smem, ...
```

**`spill stores/loads` 必须是 0**——非 0 就说明寄存器不够，变量被赶到 local memory。

## 5. 对拍

三个程序都会在主机端用三重循环独立算一遍 `ref`，打印
`C[0][0]`、`C[0][1]`、`C[1023][1023]` 三个位置的 `c_h` 与 `ref` 对比。
GPU 用 float 累加 1024 项有舍入误差，**相对误差 < 1e-3 算过**。

**红线：`matmul_reg` 跑出来比 tiled 慢 → 一定有 bug，不是「正常波动」。**
最可能的两个原因：① `acc/a/b` 掉进 local memory（用第 6 节的 `local_ld` 查）
② 循环没展开。

## 6. 用 ncu 取 Roofline 的输入

```bat
ncu --launch-count 1 --metrics dram__bytes.sum,sm__throughput.avg.pct_of_peak_sustained_elapsed,gpu__dram_throughput.avg.pct_of_peak_sustained_elapsed,smsp__sass_inst_executed_op_local_ld.sum .\matmul_reg.exe 4096
```

三个坑：

1. ncu 报的 `Duration` **不能**当性能数字（它会重放 kernel 多次），性能永远看 `cudaEvent`
2. 不加 `--launch-count 1` 会采满 4 次启动（main 是 1 预热 + 3 计时）
3. 报 `ERR_NVGPUCTRPERM` → 换管理员终端

`smsp__sass_inst_executed_op_local_ld.sum` 应为 **0**；非 0 说明寄存器数组落到了 local memory，
register tiling 白做。

## 7. 待办

- [x] `matmul_reg` 实跑并填第 1 节的空 —— 2026-11-14 完成：对拍通过、`local_ld = 0`、
      红线大胜（4.53x）。Roofline：AI=31.6 < 65.6 → memory-bound，带宽天花板完成度 64%
- [x] 四版各写一份瓶颈分析 → **[bottleneck.md](bottleneck.md)**
      （2026-11-16 晚采齐四个版本的 `dram__bytes`，11-17 写完四段分析。
      四版在 Roofline 上从 AI=14.7 一路推到 115.5，8×8 是唯一越过 ridge 的）
- [x] 8×8 register tiling 写完并编译通过（2026-11-16，`spill = 0`）
      block(32,8)、tile 64×256、128 reg/thread、occupancy 33%
- [ ] **8×8 实跑**：对拍 → 性能。三种结果都有效（更快 / 持平 / 更慢），
      load/FMA 从 0.5 降到 0.25，但 occupancy 从 83% 掉到 33%，谁更值钱要实测
