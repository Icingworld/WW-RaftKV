#pragma once

#include <string>

#include <Common.h>

namespace WW
{

/**
 * @brief 节点网络信息
*/
class RaftPeerNet
{
private:
    NodeId _Id;
    std::string _Ip;
    std::string _Port;

public:
    RaftPeerNet(NodeId _Id, const std::string & _Ip, const std::string & _Port);

    ~RaftPeerNet() = default;

public:
    /**
     * @brief 获取节点 ID
    */
    NodeId getId() const;

    /**
     * @brief 获取节点地址
    */
    const std::string & getIp() const;

    /**
     * @brief 获取节点端口
    */
    const std::string & getPort() const;
};

} // namespace WW
