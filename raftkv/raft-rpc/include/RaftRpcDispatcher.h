#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include <Logger.h>
#include <RaftRpcCommon.h>

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
        std::unique_ptr<google::protobuf::Service> _Service;        // 服务
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> _Method_map; // 方法名-方法描述
    };

private:
    std::string _Ip;                                                // IP
    std::string _Port;                                              // 端口
    std::unordered_map<std::string, ServiceInfo> _Service_map;      // 服务名-服务信息
    std::shared_ptr<muduo::net::EventLoop> _Event_loop;             // 事件循环
    std::unique_ptr<muduo::net::TcpServer> _Server;                 // Tcp Server

    Logger & _Logger;

public:
    RaftRpcDispatcher(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port);

    ~RaftRpcDispatcher() = default;

public:
    /**
     * @brief 注册服务
     * @param _Service 注册的服务，继承自`google::protobuf::Service`，由服务端实现 
     */
    void registerService(std::unique_ptr<google::protobuf::Service> _Service);

    /**
     * @brief 启动分发器
     */
    void start();

private:
    /**
     * @brief 连接事件回调函数
     * @param _Conn Tcp 连接
     */
    void _OnConnection(const muduo::net::TcpConnectionPtr & _Conn);

    /**
     * @brief 消息事件回调函数
     * @param _Conn Tcp 连接
     * @param _Buffer 消息缓冲区
     * @param _Receive_time 接收时间
     */
    void _OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time);

    /**
     * @brief 发送响应
     * @param _Conn Tcp 连接
     * @param _Service_name 服务名
     * @param _Method_name 方法名
     * @param _Sequence_id 请求序列号
     * @param _Response 响应
     * @param _Controller 控制器
    */
    void _SendResponse(const muduo::net::TcpConnectionPtr & _Conn, const std::string & _Service_name,
                    const std::string & _Method_name, SequenceType _Sequence_id,
                    google::protobuf::Message * _Response, const google::protobuf::RpcController * _Controller);
};

/**
 * @brief KV 操作服务分配器
*/
class KVOperationDispatcher
{
public:
    /**
     * @brief 服务信息
     */
    class ServiceInfo
    {
    public:
        std::unique_ptr<google::protobuf::Service> _Service;        // 服务
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> _Method_map;    // 方法名-方法描述
    };

private:
    std::string _Ip;                                                // IP
    std::string _Port;                                              // 端口
    std::unordered_map<std::string, ServiceInfo> _Service_map;      // 服务名-服务信息
    std::shared_ptr<muduo::net::EventLoop> _Event_loop;             // 事件循环
    std::unique_ptr<muduo::net::TcpServer> _Server;                 // Tcp Server

    Logger & _Logger;

public:
    KVOperationDispatcher(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port);

    ~KVOperationDispatcher() = default;

public:
    /**
     * @brief 注册服务
     * @param _Service 注册的服务，继承自`google::protobuf::Service`，由服务端实现 
     */
    void registerService(std::unique_ptr<google::protobuf::Service> _Service);

    /**
     * @brief 启动分发器
     */
    void start();

private:
    /**
     * @brief 连接事件回调函数
     * @param _Conn Tcp 连接
     */
    void _OnConnection(const muduo::net::TcpConnectionPtr & _Conn);

    /**
     * @brief 消息事件回调函数
     * @param _Conn Tcp 连接
     * @param _Buffer 消息缓冲区
     * @param _Receive_time 接收时间
     */
    void _OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time);

    /**
     * @brief 发送响应
     * @param _Conn Tcp 连接
     * @param _Service_name 服务名
     * @param _Method_name 方法名
     * @param _Sequence_id 请求序列号
     * @param _Response 响应
     * @param _Controller 控制器
    */
    void _SendResponse(const muduo::net::TcpConnectionPtr & _Conn, const std::string & _Service_name,
                    const std::string & _Method_name, SequenceType _Sequence_id,
                    google::protobuf::Message * _Response, const google::protobuf::RpcController * _Controller);
};

} // namespace WW
