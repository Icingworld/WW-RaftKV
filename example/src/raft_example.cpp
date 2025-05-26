#include <iostream>

#include <RaftClerk.h>

int main(int argc, char ** argv)
{
    if (argc != 2) {
        printf("param num error\n");
    }

    char * node_id = argv[1];

    // 创建一些节点
    std::vector<WW::RaftPeerNet> peers;
    for (int i = 0; i < 9; ++i) {
        int port = 4396 + i;
        peers.emplace_back(i, "127.0.0.1", std::to_string(port));
    }


    // 创建 Raft
    WW::RaftClerk raft(std::stoi(node_id), peers);

    raft.run();
}