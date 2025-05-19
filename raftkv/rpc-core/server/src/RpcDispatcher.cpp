#include "RpcDispatcher.h"

#include <memory>
#include <functional>

#include <ServiceRegistry.h>
#include <RpcSerialization.h>
#include <RpcConfig.h>
#include <muduo/net/InetAddress.h>

namespace WW
{

void RpcDispatcher::registerService(google::protobuf::Service * service)
{
    // 获取服务的元信息
    const google::protobuf::ServiceDescriptor * service_dsc = service->GetDescriptor();
    std::string service_name = service_dsc->name();
    int method_count = service_dsc->method_count();

    // 创建服务信息实例
    ServiceInfo info;

    for (int i = 0; i < method_count; ++i) {
        // 依次读取方法信息，并存入表
        const google::protobuf::MethodDescriptor * method_dsc = service_dsc->method(i);
        info._Method_map[method_dsc->name()] = method_dsc;
    }

    // 保存信息
    info._Service = service;
    _Service_map[service_name] = info;
}

void RpcDispatcher::run()
{
    // 获取 Zookeeper 服务注册实例
    ServiceRegistry & registry = ServiceRegistry::getServiceRegistry(
        RpcConfig::getRpcConfig().getZookeeperIp(), RpcConfig::getRpcConfig().getZookeeperPort()
    );

    // 将服务全部注册到 ZooKeeper 中
    for (auto service_it = _Service_map.begin(); service_it != _Service_map.end(); ++service_it) {
        std::string service_name = service_it->first;
        ServiceInfo & serviceInfo = service_it->second;

        for (auto method_it = serviceInfo._Method_map.begin(); method_it != serviceInfo._Method_map.end(); ++method_it) {
            if (!registry.registerService(service_name, method_it->first, 
                RpcConfig::getRpcConfig().getLocalIp(), RpcConfig::getRpcConfig().getLocalPort()
            )) {
                // 注册失败
                // TODO
            }
        }
    }

    // 启动 TCP 服务并监听
    muduo::net::InetAddress address(RpcConfig::getRpcConfig().getLocalIp(), std::stoi(RpcConfig::getRpcConfig().getLocalPort()));
    std::shared_ptr<muduo::net::TcpServer> server = std::make_shared<muduo::net::TcpServer>(&_Event_loop, address, "RpcDispatcher");

    // 设置回调函数
    server->setConnectionCallback(std::bind(&RpcDispatcher::onConnection, this, std::placeholders::_1));
    server->setMessageCallback(std::bind(&RpcDispatcher::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 启动
    server->start();
    _Event_loop.loop();
}

void RpcDispatcher::onConnection(const muduo::net::TcpConnectionPtr & conn)
{
    if (!conn->connected()) {
        // 连接关闭，断开连接
        conn->shutdown();
    }
}

void RpcDispatcher::onMessage(const muduo::net::TcpConnectionPtr & conn, muduo::net::Buffer * buffer, muduo::Timestamp receive_time)
{
    // 反序列化提取信息
    std::string recv_buf = buffer->retrieveAllAsString();
    std::string service_name;
    std::string method_name;
    std::string arg_buf;

    if (!RpcSerialization::deserialize(recv_buf, service_name, method_name, arg_buf)) {
        return;
    }

    // 从表中查找服务和方法
    auto service_it = _Service_map.find(service_name);
    if (service_it == _Service_map.end()) {
        return;
    }

    ServiceInfo & info = service_it->second;

    auto method_it = info._Method_map.find(method_name);
    if (method_it == info._Method_map.end()) {
        return;
    }

    // 创建请求信息结构
    // TODO 生命周期管理
    google::protobuf::Message * request = info._Service->GetRequestPrototype(method_it->second).New();

    if (!RpcSerialization::deserializeArgs(arg_buf, request)) {
        return;
    }

    // 创建响应结构
    // TODO 生命周期管理
    google::protobuf::Message * response = info._Service->GetResponsePrototype(method_it->second).New();

    // 创建回调函数
    google::protobuf::Closure * done = google::protobuf::NewCallback<
        RpcDispatcher, const muduo::net::TcpConnectionPtr &, google::protobuf::Message *
    >(this, &RpcDispatcher::sendResponse, conn, response);

    // 调用方法
    info._Service->CallMethod(method_it->second, nullptr, request, response, done);
}

void RpcDispatcher::sendResponse(const muduo::net::TcpConnectionPtr & conn, google::protobuf::Message * response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str)) {
        // 序列化成功，发送响应
        conn->send(response_str);
    }

    // 关闭连接
    conn->shutdown();
}

} // namespace WW
