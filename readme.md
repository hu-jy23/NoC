# Lab4 - Network-on-Chip Simulation with gem5 Garnet

本实验基于 gem5 的 Garnet 网络模块，完成了多种拓扑结构的实现与测试，包括 **Ring (1D Torus)** 和 **3D Mesh (Mesh3D_XYZ)**。同时，我们对部分配置脚本和路由单元逻辑进行了修改，以支持新的实验需求。

## ✅ 实验任务列表（按主线划分）

### 🔹 主线 A：Cluster3D（共享垂直链路 / Hub-Cluster）

- ✅ 实现 `Cluster3D_Hub.py` 拓扑文件
- [ ] 支持 cluster 尺寸配置（如 2×2×1, 2×2×2, 3×3×1）
- [ ] 实现三种簇内连接方式：Bus、Star、Mini-crossbar
- ✅ 在 TABLE_ 路由中设置权重，引导流量经 Hub 路由
- [ ] 在相同垂直链路预算下对比不同 cluster 尺寸的性能

### 🔹 主线 B：Sparse-Vertical 3D（稀疏垂直柱）

- ✅ 实现 `Sparse3D_Pillars.py` 拓扑文件
- ✅ 支持可调 pillar 间距（Px, Py ∈ {1,2,4}）
- [ ] 实现整层对齐与交错（staggered）两种柱布局
- ✅ 使用 TABLE_ 路由策略，优先引导至最近 pillar
- [ ] 绘制“垂直链路数量 vs. 平均延迟”关系曲线

### 🔹 主线 C：3D Small-World Express（小世界捷径）

- ✅ 实现 `SW3D_Express.py` 拓扑文件
- ✅ 支持 express 链路预算控制（B ∈ {0, 0.5N, 1N, 2N}）
- [ ] 实现规则式（grid-based）和随机式（random）布线策略
        只写了规则式
- ✅ 在 TABLE_ 路由中为 express 链路分配低权重
- [ ] 分析 express 链路对低/高注入率下的性能增益

### 🔹 主线 D（加分项）：Hier-3D Chiplet（分层/芯粒化）

- [ ] 实现 `Hier3D_Chiplet.py` 拓扑文件
- [ ] 支持 chiplet 尺寸配置（如 4×4×1, 4×4×2）
- [ ] 支持每 chiplet 1 或 2 个网关（gateway）路由器
- [ ] 构建上层骨干网络连接各 chiplet 网关
- [ ] 分析层次化结构对局部性与扩展性的影响

### 🧪 共享实验任务

- [ ] 配置多流量模式：uniform_random、transpose、hotspot、shuffle
- [ ] 扫描注入率（0.01 → 0.25），记录平均延迟与吞吐
- [ ] 提取关键指标：平均跳数、链路利用率、饱和点
- [ ] 对比各拓扑在相同资源预算下的性能差异
- [ ] 生成延迟-注入率曲线图与对比表格
- [ ] 撰写实验报告，突出结构设计的权衡与洞察
- [ ] 整理代码与实验数据，确保可复现性



## 📂 文件与改动说明


### 2. `configs/topologies/Mesh3D_XYZ.py`
- **用途**：新增的拓扑文件，实现了 **三维 Mesh (4×4×4 等规模)** 拓扑。
- **主要改动**：
  - 定义了 3D 网格中的坐标系统 (x, y, z)。
  - 配置了 X、Y、Z 三个方向的链路。
  - 支持通过 `--topology=Mesh3D_XYZ_` 参数调用。

---

### 3. `configs/network/Network.py`
- **用途**：注册并管理可用的拓扑结构。
- **主要改动**：
  - 在 `define_options()` 中增加了 `--topology=Ring` 和 `--topology=Mesh3D_XYZ_` 的选项。
  - 确保实验脚本能够正确识别并加载新增的拓扑。

---

### 4. `src/mem/ruby/network/garnet/RoutingUnit.cc`
- **用途**：管理路由算法的选择与调试信息输出。
- **主要改动**：
  - 增加了打印信息，方便确认所选路由算法，例如：
    ```cpp
    if (m_router->get_id() == 0 && route.hops_traversed == 0) {
        const char* alg_name = "TABLE_DOR";
        if (routing_algorithm == XY_)  alg_name = "XY_2D";
        if (routing_algorithm == XYZ_) alg_name = "XYZ_DOR_3D";
        if (routing_algorithm == CUSTOM_) alg_name = "CUSTOM";
        static bool printed = false;
        if (!printed) {
            warn("Garnet routing algorithm = XYZ_DOR_3D (id=3)");
            printed = true;
        }
    }
    ```

---

---

## 🧪 实验说明
- **目标**：比较 2D Mesh 与 3D Mesh 的拥塞特性，验证 3D 架构在高流量下具有更低的阻滞延迟。
- **关键参数**：
  - `--link-latency` 和 `--router-latency` 控制网络物理延迟（本实验默认取相同值以消除物理距离差异）。
  - `--injectionrate` 扫描不同流量强度，绘制延迟-吞吐曲线。
- **预期现象**：
  - 在相同 `link-latency` 下，3D Mesh 由于平均跳数更短，延迟随流量增加的拐点更晚出现。

---

## 📊 推荐分析指标
- **平均延迟** (`average_packet_latency`)
- **平均跳数** (`average_hops`)
- **链路利用率** (`link_utilization`)


# ---------------------------------------- #





# 总体目标

* 围绕“**三维 NoC 的垂直链路预算、结构分层与捷径连线**”这三类结构性创新，设计 3–4 个**小而美**的 3D 拓扑，**都能在 `configs/topologies/*.py` 里落地**，路由侧**尽量沿用 TABLE\_/DOR（省事且更稳）**。
* 做**参数可调**的版本：不同 cluster 大小、垂直柱（pillar）间距、express 链路预算、链路延迟映射等，保证结果“可画曲线、可分析梯度”。

---

## 主线 A：Cluster3D（共享垂直链路 / Hub-Cluster）

**想法**：每 2×2×2（或 2×2×1）个节点共享一个“簇内 hub”，由 hub 接入 Up/Down；簇内水平连接为小十字或星形。你之前的“4 节点共享”正是该思路的 2D 版，扩成 3D 很自然。

**两个可调旋钮**

1. **Cluster 尺寸**：`2×2×1`、`2×2×2`、`3×3×1`（对比 TSV 数下降 vs. 簇内仲裁开销增加）
2. **簇内连线**：

   * **Bus/Arbiter**（实现最简单；带来仲裁瓶颈）
   * **Star/Point-to-Point 到 hub**（更高带宽，额外端口）
   * **Mini-crossbar**（可视作 Star 的等价抽象）

**实现要点**

* 新建 `Cluster3D_Hub.py`：

  * 先生成“水平路由器（HR）”网格；每个 cluster 分配一个 **Hub Router（HBR）**（可以就是 HR 的一类特化，实际上还是 `Router`）。
  * 对 cluster 内节点 `ExtLink` 挂到对应 HR/HBR。
  * 只在 HBR 上创建 `Up/Down IntLink`。
* 路由：先**表驱动 `TABLE_`**（`--routing-algorithm=0`），拓扑文件里用 **权重**鼓励“Local→HBR→垂直→目标 HBR→Local”。这样不用改 C++。
* 指标：在 **相同 Up/Down 数量预算**下，对比不同 cluster 尺寸与簇内连线型式的延迟/吞吐/能耗。

---

## 主线 B：Sparse-Vertical 3D（垂直柱最小化/规则抽稀）

**想法**：不是每个节点都有 Up/Down，仅在棋盘格或固定 pitch 位置布置“**垂直柱（pillar）**”，其他节点先水平靠近柱再上/下。对应论文里的 CMIT/CIT 思想的简化工程版。

**两个可调旋钮**

1. **柱间距（pitch）**：`Px, Py ∈ {1,2,4}`（1=每个都有；2=每隔一个；4=更稀疏）
2. **柱的层间对齐**：

   * **整层对齐**（每层相同坐标为柱）
   * **交错（staggered）**（减少某些热点路径的堆叠）

**实现要点**

* 新建 `Sparse3D_Pillars.py`：

  * 仍然生成完整 3D Router 网格，但**只给满足 (x%Px==0 && y%Py==0) 的路由器加 Up/Down**；
  * 其他路由器没有 Up/Down。
* 路由：**`TABLE_`** 或 **自定义 `XYZ_`** 皆可。推荐 `TABLE_` 简化。
* 指标：**垂直链路数量 vs. 性能/能耗**曲线、pitch 扫描；再加**热点/非均匀流量**看鲁棒性。

---

## 主线 C：3D Small-World Express（小世界捷径）

**想法**：在 3D Mesh 基础上，用**小预算**加入少量“表达（express）捷径链路”，比如：

* **层内 diagonal/跨两跳**（`(x,y,z) → (x±2,y±2,z)`）
* **跨层捷径**：在少数柱上新增**直通多层**或**斜向跨层**（比如 `(x,y,z) → (x±1,y±1,z±1)`）。

**两个可调旋钮**

1. **Express 数量预算**：总数 `B ∈ {0, 0.5N, 1N, 2N}`（N=节点数）
2. **选点策略**：

   * **规则式**（每隔 K 个路由器布一条）
   * **随机式**（固定 seed，复现实验）

**实现要点**

* 新建 `SW3D_Express.py`：在生成基础 3D Mesh 后，**额外 append 一批 `IntLink`** 即可。
* 路由：**`TABLE_`**（表驱动天然能把 express 作为候选路径，用较小权重引导）或轻改 `XYZ_` 为“先尝试朝 express 方向”。
* 指标：在 **相同总线宽/线长预算**下，对比 express 数量对**低注入/临界点/饱和区**的影响；画**收益-成本**曲线。

## 快速对比基线 vs. express

建议立刻做一组对比（相同注入率、相同 cycles）：

```bash
# Baseline 3D Mesh
./build/NULL/gem5.opt -d runs_baseline \
  configs/example/garnet_synth_traffic.py \
  --network=garnet --topology=Mesh3D_XYZ_ \
  --num-cpus=64 --num-dirs=64 --mesh-rows=4 \
  --routing-algorithm=3 \
  --synthetic=uniform_random --injectionrate=0.05 \
  --sim-cycles=2000000 \
  --sys-clock=1GHz --ruby-clock=1GHz \
  --link-width-bits=128 --link-latency=2 --router-latency=2 \
  --mem-type=SimpleMemory --mem-channels=64 --mem-size=8192MB

# SW3D_Express（express 权重=0 版本）
./build/NULL/gem5.opt -d runs_swexp_w0 \
  configs/example/garnet_synth_traffic.py \
  --network=garnet --topology=SW3D_Express \
  --num-cpus=64 --num-dirs=64 --mesh-rows=4 \
  --routing-algorithm=0 \
  --synthetic=uniform_random --injectionrate=0.05 \
  --sim-cycles=2000000 \
  --sys-clock=1GHz --ruby-clock=1GHz \
  --link-width-bits=128 --link-latency=2 --router-latency=2 \
  --mem-type=SimpleMemory --mem-channels=64 --mem-size=8192MB
```

然后用你的 `collect.sh / plot_metrics.py` 画出延迟-注入率曲线，重点看：

* `average_packet_latency` 是否下降；
* `average_hops` 是否从 \~3.75 明显降低（≥5% 就很直观）；
* `packets_received/accepted_rate` 在饱和前区段有无提升。

如果你把 `W_EXP` 改成 0 之后，平均跳数仍不变，那就加大 **express 跳距/覆盖度** 或改规则式布局参数（例如每 2 个坐标放一条跨两跳的对角线），让它确实能缩短距离。













---

## 可选主线 D（加分项）：Hier-3D Chiplet（分层/芯粒化）

**想法**：把 64 节点分为 2×2×L 的“chiplet”，chiplet 内是小 3D Mesh；chiplet 之间只在**网关**路由器互连，形成上层稀疏 3D。适合现实里“多芯粒 3D 叠堆 + 互连管脚有限”的设定。

**两个可调旋钮**

1. **每个 chiplet 尺寸**：`4×4×1` vs. `4×4×2`
2. **网关数量**：每个 chiplet 1 或 2 个

**实现要点**

* 新建 `Hier3D_Chiplet.py`：

  * 先分组创建“子网格”与 gateway；
  * 再创建“上层骨干”链接 gateway 间的连线。
* 路由：先用 **`TABLE_`**；必要时把 **gateway 出口权重**设低，优先走骨干。
* 指标：层次化结构的**可扩展性**、**局部-远程访问差异**。

---

# 实验矩阵（给报告的“方法+结果”一站式清单）

* **流量**：`uniform_random`、`transpose`、`hotspot`、`shuffle`（至少 3 个）
* **注入率**：0.01 → 0.25（步长 0.01/0.02），记录**平均延迟**、**吞吐**、**saturation 点**
* **网络规模**：固定 64 节点（4×4×4）对比不同拓扑；可加 128（4×4×8 or 8×4×4）作可扩展性点
* **链路延迟映射**（可选）：

  * **同长**：所有链路 `link-latency=1`（突出拓扑结构差异）
  * **物理映射**：长链路（express/跨层多跳）设更高 `link-latency`（更贴近物理）

---

# 代码与运行落地（你可以直接按这个目录组织）

```
configs/topologies/
  Mesh3D_XYZ_.py            # 已有
  Cluster3D_Hub.py          # 主线A
  Sparse3D_Pillars.py       # 主线B
  SW3D_Express.py           # 主线C
```

**每个 .py 模板骨架（要点）**：

* 继承 `SimpleTopology`，实现 `makeTopology(self, options, network, IntLink, ExtLink, Router)`
* 按需解析自定义 CLI 选项（如 `--mesh-depth`、`--cluster-size`、`--pillar-pitch`、`--express-budget`），也可先写死参数，后续再加
* 创建 `routers=[]`，`ext_links=[]`，`int_links=[]`，**注意 link\_id 连续**
* 对 TABLE\_ 路由，合理设置 `weight` 引导路径
* 保持与 `Mesh_XY.py` 相同的外设连接逻辑（把 controllers 均匀映射到路由器）
* 运行命令示例（以主线B为例）：

  ```bash
  ./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py \
    --network=garnet \
    --topology=Sparse3D_Pillars \
    --num-cpus=64 --num-dirs=64 \
    --mesh-rows=4 \
    --routing-algorithm=0 \              # TABLE_
    --synthetic=uniform_random \
    --injectionrate=0.05 \
    --sim-cycles=1000000 \
    --sys-clock=1GHz --ruby-clock=1GHz \
    --link-width-bits=128 \
    --link-latency=1 --router-latency=1
  ```

---

# 10 天排期（两人）

* **D1–D2**：落地主线 A（Cluster3D\_Hub.py，先 2×2×1 + bus），跑通 & 画一条曲线
* **D3–D4**：加 2×2×2/3×3×1 变体 + star 变体，做 pitch/结构消融
* **D5–D6**：落地主线 B（Sparse3D\_Pillars.py），做 pitch 扫描
* **D7**：落地主线 C（SW3D\_Express.py，先规则式 express），跑 express 预算扫描
* **D8**：把 3 条主线用相同脚本批量跑（uniform/transpose/hotspot），导出 CSV
* **D9**：画图（延迟–注入率、吞吐–注入率、饱和点对比、预算–收益曲线），初版报告
* **D10**：补充一组“物理映射”的链路延迟实验（可选）+ 完善写作

---

# 报告里可强调的“研究性点”

* **TSV/垂直链路预算 vs. 性能** 的量化权衡（主线 A/B）
* **小世界 express** 的“**微量长链路带来的宏观收益**”与性价比（主线 C）
* **簇内结构**（bus/star）对**低负载/高负载**两端的不同影响
* **dateline/VC 类**为何在 Torus 必要（理论说明即可，展示你理解全栈）

---

如果你愿意，我可以马上给你\*\*`Sparse3D_Pillars.py` 的最小可跑版本\*\*（带 `Px,Py` 两个参数），以及 **`SW3D_Express.py` 的规则式 express 代码骨架**。你贴到 `configs/topologies/` 里直接跑就行。
