
#pragma once

#include <ZooKeeperClient.h>

namespace WW
{

/**
 * @brief 服务发现
 */
class ServiceDiscovery
{
private:
    ZooKeeperClient _Zk_client;     // ZooKeeper 客户端

private:
    ServiceDiscovery(const std::string & ip, const std::string & port, int timeout = 3000);

    ServiceDiscovery() = delete;

    ServiceDiscovery(const ServiceDiscovery & other) = delete;

    ServiceDiscovery & operator=(const ServiceDiscovery & other) = delete;

public:
    ~ServiceDiscovery() = default;

public:
    /**
     * @brief 获取服务发现单例
     * @param ip ZooKeeper 服务地址
     * @param port ZooKeeper 服务端口
     * @param timeout 超时时间
     * @return 服务发现单例
     */
    static ServiceDiscovery & getServiceDiscovery(const std::string & ip, const std::string & port, int timeout = 3000);

    /**
     * @brief 发现服务
     * @param service_name 服务名
     * @param method_name 方法名
     * @return 储存`ip:port`字符串的数组
     */
    std::vector<std::string> discoverService(const std::string & service_name, const std::string & method_name);
};

} // namespace WW
