#pragma once

#include <string>

namespace WW
{

/**
 * @brief RPC 配置
*/
class RpcConfig
{
private:
    RpcConfig();

    RpcConfig(const RpcConfig & other) = delete;

    RpcConfig & operator=(const RpcConfig & other) = delete;

public:
    ~RpcConfig();

public:
    static RpcConfig & getRpcConfig();

    std::string getZookeeperHost();

    std::string getZookeeperIp();

    std::string getZookeeperPort();

    int getZookeeperTimeout();

    std::string getLocalHost();

    std::string getLocalIp();

    std::string getLocalPort();
};

} // namespace WW
