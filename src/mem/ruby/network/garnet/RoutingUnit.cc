/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {
        // 多个 NI 可能接在本 router 上，Local 出口号从路由表拿
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // 选择的路由算法（由 --routing-algorithm 传入）
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    // —— 只打印一次所选算法，避免刷屏，也消除“未使用变量”告警 ——
    static bool printed = false;
    if (!printed) {
        const char* alg_name = "TABLE_DOR";
        switch (routing_algorithm) {
            case TABLE_:  alg_name = "TABLE_DOR";    break;
            case XY_:     alg_name = "XY_2D";        break;
            case XYZ_:    alg_name = "XYZ_DOR_3D";   break;
            case CUSTOM_: alg_name = "CUSTOM";       break;
            default:      alg_name = "UNKNOWN";      break;
        }
        warn("Garnet routing algorithm = %s (id=%d)",
             alg_name, (int)routing_algorithm);
        printed = true;
    }
    // —— 打印区到此结束，注意这里有个右花括号！ ——

    // 真正的分派
    switch (routing_algorithm) {
        case TABLE_:
            outport = lookupRoutingTable(route.vnet, route.net_dest);
            break;
        case XY_:
            outport = outportComputeXY(route, inport, inport_dirn);
            break;
        case XYZ_:
            outport = outportComputeXYZ(route, inport, inport_dirn);
            break;
        case CUSTOM_:
            outport = outportComputeCustom(route, inport, inport_dirn);
            break;
        default:
            outport = lookupRoutingTable(route.vnet, route.net_dest);
            break;
    }

    assert(outport != -1);
    return outport;
}


// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

//handmade：
// XYZ routing implemented using port directions
// Only for reference purpose in a 3D Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXYZ(RouteInfo route,
                               int inport,
                               PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    // ❌ 不要这样用 2D 的 num_cols 当 X
    // int num_cols = m_router->get_net_ptr()->getNumCols();

    // ✅ 3D 情况下，用 rows 当 Y，并假设每层是方阵：X=Y
    int Y = m_router->get_net_ptr()->getNumRows(); // = mesh_rows
    assert(Y > 0);
    int X = Y;  // 方阵层：X=Y
    int num_routers = m_router->get_net_ptr()->getNumRouters();
    assert((num_routers % (X * Y)) == 0);
    int Z = num_routers / (X * Y);
    assert(Z > 0);

    // 下面坐标解码与 Python 拓扑一致：id = z*(X*Y) + y*X + x
    int my_id = m_router->get_id();
    int my_x =  my_id % X;
    int my_y = (my_id / X) % Y;
    int my_z =  my_id / (X * Y);

    int dest_id = route.dest_router;
    int dest_x =  dest_id % X;
    int dest_y = (dest_id / X) % Y;
    int dest_z =  dest_id / (X * Y);

    int dx = dest_x - my_x;
    int dy = dest_y - my_y;
    int dz = dest_z - my_z;

    // 3D DOR: X -> Y -> Z
    if (dx != 0) {
        if (dx > 0) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (dy != 0) {
        if (dy > 0) {
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else if (dz != 0) {
        if (dz > 0) {              // 需要往上层走
            assert(inport_dirn != "Up");
            outport_dirn = "Up";
        } else {                   // 需要往下层走
            assert(inport_dirn != "Down");
            outport_dirn = "Down";
        }
    } else {
        panic("XYZ: already at destination router");
    }

    return m_outports_dirn2idx[outport_dirn];
}





// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
int
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;
    int num_routers = m_router->get_net_ptr()->getNumRouters();

    int clockwise_dist = (dest_id - my_id + num_routers) % num_routers;
    int counter_dist = (my_id - dest_id + num_routers) % num_routers;

    PortDirection outport_dirn;

    if (clockwise_dist <= counter_dist) {
        outport_dirn = "East";  // 顺时针走
    } else {
        outport_dirn = "West";  // 逆时针走
    }

    return m_outports_dirn2idx[outport_dirn];
}


} // namespace garnet
} // namespace ruby
} // namespace gem5
