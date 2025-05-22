#pragma once

#include <string>

#include <Common.h>

namespace WW
{

/**
 * @brief 同伴
*/
class RaftPeer
{
private:
    NodeId _Id;             // 节点 ID
    std::string _Ip;        // IP 地址
    std::string _Port;      // 端口

public:
    RaftPeer(NodeId _Id, const std::string & _Ip, const std::string & _Port);

public:
    /**
     * @brief 获取节点 ID
    */
    NodeId getId() const;

    /**
     * @brief 获取同伴 IP 地址
    */
    const std::string & getIp() const;

    /**
     * @brief 获取同伴端口
    */
    const std::string & getPort() const;
};

} // namespace WW
