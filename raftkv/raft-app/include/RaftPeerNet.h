#pragma once

#include <string>

#include <RaftCommon.h>

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
    std::string _KV_port;

public:
    RaftPeerNet(NodeId _Id, const std::string & _Ip, const std::string & _Port, const std::string & _KV_port);

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

    /**
     * @brief 获取 KV 操作服务端口
    */
    const std::string & getKVPort() const;
};

} // namespace WW
