#include <Raft.h>

int main()
{
    // 创建一些节点
    std::vector<WW::RaftPeer> peers;
    peers.emplace_back(0, "127.0.0.1", "4396");
    peers.emplace_back(1, "127.0.0.1", "4397");
    peers.emplace_back(2, "127.0.0.1", "4398");
    peers.emplace_back(3, "127.0.0.1", "4399");
    peers.emplace_back(4, "127.0.0.1", "4400");

    // 创建 Raft
    WW::Raft raft(0, peers);

    // 启动 Raft
    raft.run();
}