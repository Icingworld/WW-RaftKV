#pragma once

#include <string>
#include <unordered_map>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>

namespace WW
{

/**
 * @brief 服务信息
 */
class ServiceInfo
{
public:
    google::protobuf::Service * _Service;       // 服务
    std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> _Method_map; // 方法名-方法描述
};

/**
 * @brief 服务分发
 */
class RpcDispatcher
{
private:
    std::unordered_map<std::string, ServiceInfo> _Service_map;      // 服务名-服务信息
    muduo::net::EventLoop _Event_loop;                              // 事件循环      

public:

public:
    /**
     * @brief 注册服务
     * @param service 注册的服务，继承自`google::protobuf::Service`，由服务端实现并传入
     */
    void registerService(google::protobuf::Service * service);

    /**
     * @brief 启动分发器
     */
    void run();

private:
    /**
     * @brief 连接事件回调函数
     * @param conn Tcp 连接
     */
    void onConnection(const muduo::net::TcpConnectionPtr & conn);

    /**
     * @brief 消息事件回调函数
     */
    void onMessage(const muduo::net::TcpConnectionPtr & conn, muduo::net::Buffer * buffer, muduo::Timestamp receive_time);

    /**
     * @brief 发送响应
     */
    void sendResponse(const muduo::net::TcpConnectionPtr & conn, google::protobuf::Message * response);
};

} // namespace WW
