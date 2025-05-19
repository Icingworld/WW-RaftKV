#pragma once

#include <ZooKeeperClient.h>

namespace WW
{

/**
 * @brief 服务注册
 */
class ServiceRegistry
{
private:
    ZooKeeperClient _Zk_client;     // ZooKeeper 客户端

private:
    ServiceRegistry(const std::string & ip, const std::string & port, int timeout = 3000);

    ServiceRegistry() = delete;

    ServiceRegistry(const ServiceRegistry & other) = delete;

    ServiceRegistry & operator=(const ServiceRegistry & other) = delete;

public:
    ~ServiceRegistry() = default;

public:
    /**
     * @brief 获取服务注册单例
     * @param ip ZooKeeper 服务地址
     * @param port ZooKeeper 服务端口
     * @param timeout 超时时间
     * @return 服务注册单例
     */
    static ServiceRegistry & getServiceRegistry(const std::string & ip, const std::string & port, int timeout = 3000);

    /**
     * @brief 注册服务
     * @param service_name 服务名
     * @param method_name 方法名
     * @param ip 服务所在地址
     * @param port 服务所在端口
     */
    bool registerService(const std::string & service_name, const std::string & method_name,
                         const std::string & ip, const std::string & port);
};

} // namespace WW
