#include <iostream>
#include <cstdlib>

#include <Raft.h>

int main(int argc, char ** argv)
{
    if (argc < 2) {
        std::cerr << "Too few parameters" << std::endl;
        return -1;
    }

    if (argc > 2) {
        std::cerr << "Too more parameters" << std::endl;
        return -1;
    }

    int node = std::stoi(argv[1]);

    // 创建一些节点
    std::vector<WW::RaftPeer> peers;
    peers.emplace_back(0, "127.0.0.1", "4396");
    peers.emplace_back(1, "127.0.0.1", "4397");
    peers.emplace_back(2, "127.0.0.1", "4398");

    // 创建 Raft
    WW::Raft raft(node, peers);

    // 启动 Raft
    raft.run();
}