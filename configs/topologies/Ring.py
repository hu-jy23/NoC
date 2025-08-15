from m5.params import *
from m5.objects import *

from common import FileSystemConfig

from topologies.BaseTopology import SimpleTopology

class Ring(SimpleTopology):
    description = "Ring"

    def __init__(self, controllers):
        self.nodes = controllers

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes
        num_routers = options.num_cpus  # 还是 16

        if len(nodes) > num_routers:
            print(f"[Ring Topology] Detected {len(nodes)} controllers, only {num_routers} routers, assigning multiple nodes per router.")

        link_latency = options.link_latency
        router_latency = options.router_latency

        # 创建 routers
        routers = [Router(router_id=i, latency=router_latency) for i in range(num_routers)]
        network.routers = routers

        # 外部链接
        ext_links = []
        link_id = 0
        for i, node in enumerate(nodes):
            router_id = i % num_routers  # 将32个controller均匀映射到16个router上
            ext_links.append(
                ExtLink(link_id=link_id, ext_node=node, int_node=routers[router_id], latency=link_latency)
            )
            link_id += 1

        network.ext_links = ext_links

        # 内部连接（形成环）
        int_links = []
        for i in range(num_routers):
            next_i = (i + 1) % num_routers
            int_links.append(IntLink(
                link_id=link_id,
                src_node=routers[i],
                dst_node=routers[next_i],
                src_outport="East",
                dst_inport="West",
                latency=link_latency,
                weight=1
            ))
            link_id += 1

            int_links.append(IntLink(
                link_id=link_id,
                src_node=routers[next_i],
                dst_node=routers[i],
                src_outport="West",
                dst_inport="East",
                latency=link_latency,
                weight=1
            ))
            link_id += 1

        network.int_links = int_links


    def registerTopology(self, options):
        for i in range(options.num_cpus):
            FileSystemConfig.register_node(
                [i], MemorySize(options.mem_size) // options.num_cpus, i
            )