# 15-scan · 前缀和（Blelloch / Hillis-Steele）

> ROADMAP 第 13 周。硬件：RTX 4070 Ti Super（Ada，CC 8.9）。
> ⚠️ **三个版本写完并编译通过，但一个都没跑过。** 性能列待补。

## 前缀和是什么

```
输入 a  = [3, 1, 7, 0, 4, 1, 6, 3]

包含式  = [3, 4, 11, 11, 15, 16, 22, 25]     自己算进去
排除式  = [0, 3, 4, 11, 11, 15, 16, 22]      自己不算
```

**本目录实现的都是排除式**（Blelloch 天然产出）。

用途：stream compaction、radix sort、并行分区。面试高频，因为它是「输出依赖前缀」这类问题的通用解。

## 为什么并行难

每个输出依赖它前面的所有输入，看起来天然串行（O(n) 深度，GPU 上毫无并行度）。三条路：

| 算法 | 工作量 | 深度 |
|---|---|---|
| 串行 | O(n) | O(n) |
| Hillis-Steele | O(n log n) | O(log n) |
| **Blelloch** | **O(n)** | O(log n) |

Blelloch 是唯一「工作量不增加」的并行算法，代价是要扫两遍。

## 版本谱系

| 版本 | 算法 | 范围 | 说明 |
|---|---|---|---|
| `scan_v0` | Blelloch | 单 block，**分段** | 每个 block 只扫自己那 256 个，块间不传递 |
| `scan_v1` | Blelloch | 多 block，**全局** | 三个 kernel：本地扫 + 块总和扫 + 加回偏移 |
| `scan_v2` | **Hillis-Steele** | 多 block，全局 | block 内改用 warp shuffle，shared 树被替换 |

## Blelloch：上扫 + 下扫

**上扫就是归约那棵树**，只是中间结果要全部留着：

```
初始：[3, 1, 7, 0, 4, 1, 6, 3]
d=1: [3, 4, 7, 7, 4, 5, 6, 9]      a[i] += a[i-1]，i 为奇数位
d=2: [3, 4, 7, 11, 4, 5, 6, 14]    a[i] += a[i-2]，i = 3,7
d=4: [3, 4, 7, 11, 4, 5, 6, 25]    a[i] += a[i-4]，i = 7
```

通用规则：

```
第 d 层：所有满足 (i + 1) % (2d) == 0 的位置 i，执行 a[i] += a[i - d]
```

**下扫：先把根清零，再倒着走，每对做「交换再加」**

```cpp
t     = a[i1];
a[i1] = a[i2];
a[i2] = t + a[i2];
```

走完就得到排除式前缀和。**根清零是关键一步**：树根的「总和」换成「前面的总和」，
因为它前面什么都没有，所以是 0。

## Hillis-Steele：一遍往上加

```cpp
for (int off = 1; off < 32; off *= 2) {
    float y = __shfl_up_sync(0xffffffff, x, off);   // 先读旧值
    if (lane >= off) x += y;                         // 再写回
}
```

**两个要命的细节：**

1. **同一层内必须用旧值**——所以先 `shfl` 读、后 `+=` 写。一边算一边更新会全错
2. `if (lane >= off)` 不能少，warp 边界外拿到的是垃圾

`__shfl_up_sync` 和归约用的 `__shfl_down_sync` 方向相反：**up 是线程 i 拿线程 i-off 的值。**

## scan_v1 的三个 kernel

```
scan_v1_local   <<<nBlocks, 256>>>   本地 Blelloch + 记录本块总和
scan_v1_offsets <<<1, nBlocks>>>     对 block_sums 做单 block scan
scan_v1_add     <<<nBlocks, 256>>>   out[i] += block_sums[blockIdx.x]
```

**三次启动必须包进同一个计时窗口**（`Record(start)` → 三个 launch → `Record(stop)`），
中间绝不能插 `cudaMemcpy` 或同步。

**规模限定 `N = 262144`**（= 256 × 1024），保证 `nBlocks = 1024` 能用一个 block 扫完。
传更大会直接打印错误退出。

## ⚠️ 静态 `__shared__` 的大小必须是编译期常量

```cpp
__shared__ float tile[TILE];     // ✅ TILE 是 constexpr
__shared__ float tile[blockDim.x];  // ❌ 运行时值，编译不过
```

`TILE` 抽成 `constexpr int TILE = 256;`，数组声明、`n`、main 的 `dim3` 三处共用——
避免「`dim3(256)` 和 `tile[256]` 两个真相源」靠人肉保持一致。

## harness（`scan_common.h`）和归约的差别

| | reduction | scan |
|---|---|---|
| 输出大小 | `grid.x`（或 1） | **`N`** |
| 对拍方式 | 全加成一个标量比 | **逐元素比** |
| 需要 `cudaMemset` | 需要 | 不需要（全覆盖写） |

## 跑法

```bat
build.bat 15-scan run 262144
```

`scan_v0` 打印 `scan check: OK`，`scan_v1` / `scan_v2` 打印 `bad_count=0`。

**注意 N = 262144，不是归约那个 10485760** —— 两组规模不同，时间不可横向比。

## 技术债

`scan_v1_offsets` / `scan_v1_add` 在 `scan_v1.cu` 和 `scan_v2.cu` 里**各有一份**。
写 v3 时抽到 `scan_kernels.h`，三个 `.cu` 一起 include。

（先让功能对齐、再重构——和 matmul 那次一样。）

## 待写

`scan_v3.cu`（空文件）：**每线程 4 个元素**。思路——

线程内串行 scan → 用 `x[3]`（本线程总和）做 warp scan →
`__shfl_up_sync(..., carry, 1)` 转成排斥式（lane 0 补 0）→ 加回。

⚠️ **动手前先答一个问题**：`TILE` 还能是 256 吗？256 线程 × 4 元素 = 1024，
如果 TILE 仍是 256，`a[blockIdx.x*TILE + threadIdx.x*4 + i]` 会越界到什么程度？

## 没写的东西：单 kernel（decoupled look-back）

CUB 用的做法，**只讲概念、不写实现**：

1. 每块算本地 scan 和总和
2. 发布总和，标记「聚合值就绪」
3. 回头往前找前辈的聚合值，直到遇到「前缀就绪」的块
4. 得到偏移后发布自己的「前缀就绪」

难点在内存序（`__threadfence()` + volatile 轮询），写错的表现是死锁或读到旧值，
而且时灵时不灵。等 v0-v2 有数字之后再考虑。
