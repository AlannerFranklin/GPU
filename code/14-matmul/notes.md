# 14-matmul 笔记：tiled matmul 为什么只有 1.29x

> 记录时间：2026-11-10
> 4070 Ti Super + nvcc 12.6。对应 ROADMAP 第 11-12 周。实验：naive vs shared memory tiling。

## 1. 数据

| 版本 | N | min_ms | GFLOPS | 加速比 |
|---|---|---|---|---|
| naive | 1024 | 0.817344 | 2627 | 1.0x |
| tiled | 1024 | 0.635904 | 3377 | 1.29x |
| naive | 4096 | 今晚填 | | |
| tiled | 4096 | 今晚填 | | |

## 2. 为什么 tiled 只快了 1.29 倍

tiled 的读数总量还是 2N³（21 亿次），只是把其中 **31/32** 的读，
从发往 **global 地址**换成了发往 **shared 地址**。

而 A+B 只有 8 MB，L2 有 48 MB，所以 naive 的重复读本来就命中 L2，没走到 DRAM。

| 位置 | 谁在用 | 你的代码里怎么写的 |
|---|---|---|
| L2 | GPU 芯片上，全 GPU 共享 | naive 的 21 亿次读落在这 | 代码里看不见，是硬件自动的 |
| shared | 每个 SM 内部 | tiled 的 21 亿次读落在这 | `__shared__ float As[32][32]` |

**读数账（N=1024）**

| | 读 global | 读 shared | 合计 |
|---|---|---|---|
| naive | 2N³ = 21 亿（全命中 L2） | 0 | 21 亿 |
| tiled | 2N³/32 = 6700 万 | 2N³×31/32 ≈ 20 亿 | 21 亿 |

## 3. 为什么 `[32][32]` 不需要 padding

```
bank = 这个 float 在 shared 里是【第几个】 % 32
```

`As[i][j]` 是第 `i*32 + j` 个，所以 `bank = (i*32 + j) % 32 = j`。

**`As[ty][k]`**

- 第 `ty*32 + k` 个，`bank = (ty*32 + k) % 32 = k`
- 同一 warp 内 ty 固定、k 相同 → **地址完全相同** → **广播**，1 笔事务

**`Bs[k][tx]`**

- 第 `k*32 + tx` 个，`bank = (k*32 + tx) % 32 = tx`
- 同一 warp 内 tx 走 0..31 → bank 分别是 0-31 → **各不相同 → 无冲突**，1 笔事务

> **所以 As 和 Bs 都不需要 padding。**

### 为什么 13-transpose 的 32×33 教训在这里用不上

```
转置：  tile[threadIdx.x][threadIdx.y]      ← 第一维是 tx，第二维是 ty
matmul：Bs[k][threadIdx.x]                   ← 第一维是 k，第二维是 tx
```

13-transpose 读的时候，warp 里变的是**第一维**（tx），固定的在第二维（ty）
—— 相当于**按列读** row-major 数组，32 个线程撞同一个 bank，所以需要 33 列错开。

matmul 读 `Bs[k][tx]` 时，warp 里变的是**第二维**（tx），固定的在第一维（k）
—— 相当于**按行读**，是 row-major 的自然方向，bank 天然铺开，不用 padding。

## 4. 两个 `__syncthreads()` 各防什么

| 删掉哪个 | 会发生什么 | 术语 |
|---|---|---|
| 第一个（填完 shared、读之前） | 不能保证读的时候写完了，很有可能还在写入 | **RAW**（Read After Write） |
| 第二个（读完 shared、下一轮写之前） | 不能保证第二次写入之前读完了，可能会覆盖之前的数据 | **WAR**（Write After Read） |

两者都会让结果错，但程序**不崩、不报错**，只是数不对。

## 5. 下一步

现在每线程只算 1 个输出，读 2 个数才做 1 次乘加。
下一步是 **register tiling**（每线程 8×8 输出），把这个比例变成 4 次乘加 / 1 次读。
