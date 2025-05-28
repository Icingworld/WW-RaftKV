#pragma once

#include <unordered_map>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>

namespace WW
{

/**
 * @brief Raft 分配器
*/
class RaftRpcDispatcher
{
public:
    /**
     * @brief 服务信息
     */
    class ServiceInfo
    {
    public:
        google::protobuf::Service * _Service;       // 服务
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> _Method_map; // 方法名-方法描述
    };

private:
    std::string _Ip;
    std::string _Port;
    std::unordered_map<std::string, ServiceInfo> _Service_map;      // 服务名-服务信息
    muduo::net::EventLoop * _Event_loop;                            // 事件循环

public:
    RaftRpcDispatcher(const std::string & _Ip, const std::string & _Port);

public:
    /**
     * @brief 注册服务
     * @param _Service 注册的服务，继承自`google::protobuf::Service`，由服务端实现并传入
     */
    void registerService(google::protobuf::Service * _Service);

    /**
     * @brief 启动分发器
     */
    void run();

private:
    /**
     * @brief 连接事件回调函数
     * @param conn Tcp 连接
     */
    void _OnConnection(const muduo::net::TcpConnectionPtr & _Conn);

    /**
     * @brief 消息事件回调函数
     */
    void _OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time);

    /**
     * @brief 发送响应
    */
    void _SendResponse(const std::string & _Service_name, const std::string & _Method_name, const muduo::net::TcpConnectionPtr & _Conn, google::protobuf::Message * _Response);
};

/**
 * @brief Raft 分配器
*/
class RaftOperationDispatcher
{
public:
    /**
     * @brief 服务信息
     */
    class ServiceInfo
    {
    public:
        google::protobuf::Service * _Service;       // 服务
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> _Method_map; // 方法名-方法描述
    };

private:
    std::string _Ip;
    std::string _Port;
    std::unordered_map<std::string, ServiceInfo> _Service_map;      // 服务名-服务信息
    muduo::net::EventLoop * _Event_loop;                            // 事件循环

public:
    RaftOperationDispatcher(const std::string & _Ip, const std::string & _Port);

public:
    /**
     * @brief 注册服务
     * @param _Service 注册的服务，继承自`google::protobuf::Service`，由服务端实现并传入
     */
    void registerService(google::protobuf::Service * _Service);

    /**
     * @brief 启动分发器
     */
    void run();

private:
    /**
     * @brief 连接事件回调函数
     * @param conn Tcp 连接
     */
    void _OnConnection(const muduo::net::TcpConnectionPtr & _Conn);

    /**
     * @brief 消息事件回调函数
     */
    void _OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time);

    /**
     * @brief 发送响应
    */
    void _SendResponse(const std::string & _Service_name, const std::string & _Method_name, const muduo::net::TcpConnectionPtr & _Conn, google::protobuf::Message * _Response);
};

} // namespace WW
