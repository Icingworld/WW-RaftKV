#include "ServiceDiscovery.h"

namespace WW
{

ServiceDiscovery::ServiceDiscovery(const std::string & ip, const std::string & port, int timeout)
    : _Zk_client()
{
    // 连接 ZooKeeper 服务器
    _Zk_client.connect(ip + ":" + port, timeout);
}

ServiceDiscovery & ServiceDiscovery::getServiceDiscovery(const std::string & ip, const std::string & port, int timeout)
{
    static ServiceDiscovery discovery(ip, port, timeout);
    return discovery;
}

std::vector<std::string> ServiceDiscovery::discoverService(const std::string & service_name, const std::string & method_name)
{
    // 构造路径
    std::string base_path = "/" +service_name + "/" + method_name;

    std::vector<std::string> childs;

    _Zk_client.getChildren(base_path, childs);

    return childs;
}

} // namespace WW
