# Lab4 - Network-on-Chip Simulation with gem5 Garnet

本实验基于 gem5 的 Garnet 网络模块，完成了多种拓扑结构的实现与测试，包括 **Ring (1D Torus)** 和 **3D Mesh (Mesh3D_XYZ)**。同时，我们对部分配置脚本和路由单元逻辑进行了修改，以支持新的实验需求。

---

## 📂 文件与改动说明

### 1. `configs/topologies/Ring.py`
- **用途**：新增的拓扑文件，实现了 **环形 (1D Torus)** 拓扑。
- **主要改动**：
  - 定义了节点与路由器的映射关系。
  - 配置了路由器之间的环形连线。
  - 支持通过 `--topology=Ring` 参数调用。

---

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

### 5. `configs/example/garnet_synth_traffic.py`
- **用途**：实验入口脚本，用于运行合成流量 (synthetic traffic) 测试。
- **使用方式**：
  - 运行 Ring：
    ```bash
    ./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py \
      --network=garnet --topology=Ring \
      --num-cpus=16 --num-dirs=16 \
      --injectionrate=0.05 --sim-cycles=10000
    ```
  - 运行 3D Mesh (64 nodes, 4×4×4)：
    ```bash
    ./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py \
      --network=garnet --topology=Mesh3D_XYZ_ \
      --num-cpus=64 --num-dirs=64 --mesh-rows=4 \
      --routing-algorithm=3 \
      --synthetic=uniform_random \
      --injectionrate=0.05 --sim-cycles=1000000
    ```

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
