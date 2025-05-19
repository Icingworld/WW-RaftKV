#include "RpcConfig.h"

namespace WW
{

RpcConfig::RpcConfig()
{

}

RpcConfig::~RpcConfig()
{

}

RpcConfig & RpcConfig::getRpcConfig()
{
    static RpcConfig config;
    return config;
}

std::string RpcConfig::getZookeeperHost()
{
    return "127.0.0.1:2181";
}

std::string RpcConfig::getZookeeperIp()
{
    return "127.0.0.1";
}

std::string RpcConfig::getZookeeperPort()
{
    return "2181";
}

std::string RpcConfig::getLocalHost()
{
    return "127.0.0.1:6666";
}

std::string RpcConfig::getLocalIp()
{
    return "127.0.0.1";
}

std::string RpcConfig::getLocalPort()
{
    return "6666";
}

} // namespace WW
