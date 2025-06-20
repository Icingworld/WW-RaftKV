#include <iostream>
#include <signal.h>

#include <RaftClerk.h>

WW::RaftClerk * raft_clerk = nullptr;

void signalHandle(int sig)
{
    if (raft_clerk != nullptr) {
        raft_clerk->stop();
    }
}

int main(int argc, char ** argv)
{
    signal(SIGINT, signalHandle);
    signal(SIGTERM, signalHandle);

    if (argc != 2) {
        std::cerr << "param num error" << std::endl;
        return -1;
    }

    char * arg = argv[1];
    WW::NodeId node_id = std::stoi(arg);

    if (node_id < 0 || node_id > 6) {
        std::cerr << "invalid node id" << std::endl;
        return -1;
    }

    // 创建 5 个节点
    std::vector<WW::RaftPeerNet> peers;
    for (int i = 0; i < 5; ++i) {
        int port = 4396 + i * 2;
        peers.emplace_back(i, "127.0.0.1", std::to_string(port), std::to_string(port + 1));
    }

    // 创建 Raft
    raft_clerk = new WW::RaftClerk(node_id, peers);

    raft_clerk->start();
}