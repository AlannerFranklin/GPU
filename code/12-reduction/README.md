# 12-reduction · 归约的七个版本

> ROADMAP 第 13 周。硬件：RTX 4070 Ti Super（Ada，CC 8.9）。
> ⚠️ **七个版本全部写完并编译通过，但一个都没跑过。** 下方性能列待补。

把一个长度为 N 的 `float` 数组求和成一个标量。看着简单，但它同时踩到 SIMT 的四个坑：
**分支发散、同步开销、bank conflict、跨 block 合并**——比 matmul 多踩两个。

## 版本谱系

| 版本 | 干了什么 | 关键机制 |
|---|---|---|
| `v0` | 基线 | shared 树形；9 次 `__syncthreads`；每线程 1 个元素 |
| `v1` | 展开 | `s ≤ 32` 后只用一个 warp，同步降到 3 次 + 6 次 `__syncwarp` |
| `v2` | 每线程 4 个元素 | block 数除以 4；读入用**跨步** `a[base + i*blockDim.x + tid]` 保证合并 |
| `v3` | 最后 32 个改用 shuffle | 6 次 shared 往返 → 5 步寄存器传递 |
| `v4` | 整棵树干掉 | 8 个 warp 各自 shuffle；shared 只要 8 个 float；1 次同步 |
| `v5` | `float4` 向量化 | 4 条 load 指令 → 1 条 |
| `v6` | 单 kernel 完成 | `atomicAdd` 跨 block 合并，不再靠主机端求和 |

## 基线为什么慢（v0 的账）

`blockDim.x = 256` 时树有 8 轮，每轮一次 `__syncthreads`：

| 轮 | s | 干活线程 | 空闲线程 |
|---|---|---|---|
| 1 | 128 | 128 | 128 |
| 2 | 64 | 64 | 192 |
| … | … | … | … |
| 8 | 1 | 1 | 255 |

```
有效线程-轮次 = 128+64+32+16+8+4+2+1 = 255
利用率 = 255 ÷ (8 × 256) = 12.45%
```

**树形归约的加法次数是最优的（N−1 次），但并行度指数衰减，96.5% 的线程-轮次在空转，
而且每轮还要一次块级屏障。**

## 三个关键概念

**① 屏障不能放在发散的分支里**

```cpp
if (threadIdx.x < s) {
    ...
    __syncthreads();      // ❌ 只有 tid < s 的线程到达 → 内核挂死
}
__syncthreads();          // ✅ 放到 if 外面
```

判断条件里一旦出现 `threadIdx` 就危险；`blockIdx` 是全 block 一致的，安全。

**② Volta 之后 warp 内不再严格锁步**

所以 warp 内同步要用 `__syncwarp()`，不能指望 `volatile`（Harris 2007 那套是 Tesla 时代的写法）。

**③ `__shfl_down_sync(mask, v, off)`**

线程 i 拿到线程 i+off 的 v，全在寄存器之间传，不经过 shared、不需要同步。

```cpp
for (int off = 16; off > 0; off /= 2)
    sum += __shfl_down_sync(0xffffffff, sum, off);
```

**off 从 16 起**，因为 warp 里最大只有 31 号线程。只有线程 0 最后拿到完整结果。

**block 级两阶段归约（v4）**：8 个 warp 各自 shuffle 成 1 个值 → lane 0 写
`warp_sums[warp]` → `__syncthreads()` → warp 0 归约这 8 个值（`off` 从 4 起）。

⚠️ 第二阶段的 `sum = (lane < 8) ? warp_sums[lane] : 0.0f;` 不能省：
既防越界，又保证 32 个线程全员参与 shuffle。

## harness 设计（`reduction_common.h`）

和 matmul 的差别只有一处：**输出 buffer 按 `grid.x` 分配，且必须 `cudaMemset` 清零。**

原因：不同版本输出形状不同（v0-v5 每 block 一格，v6 只写 `out[0]`）。清零之后，
主机端可以无脑把 `grid.x` 个数全加起来，没写的格子是 0，不影响结果。

**判据**：这块内存在被读之前，是不是保证每个字节都被写过？
matmul 的 `c_d` 保证（N×N 全覆盖），所以不用清；归约不保证，所以要清。

## 跑法

```bat
build.bat 12-reduction run 10485760
```

**N = 10485760 = 10240 × 1024**，七版同一个 N 才能横向比（v0/v1 要能被 256 整除，
v2 以后要能被 1024 整除）。

要看的三个数：

| | 判定 |
|---|---|
| `rel_err` | 七个都 < 1e-3。**任何一个大了，那一版有 bug，性能数字无意义** |
| `sum` | 七版应基本相同 |
| `min_ms` | 预期 v0 > v1 > v2 > v3 ≈ v4 > v5，**v6 可能回升** |

## v6 为什么可能更慢（不是 bug）

1. **测量边界**：harness 只计 kernel 时间。v0-v5 的主机端求和发生在计时结束之后，
   v6 的原子加发生在 kernel 内部
2. **原子竞争**：10240 个 block 抢同一个地址，硬件会串行化

它完成了一件 v0-v5 没做的事——把归约真正在 GPU 上做完。
