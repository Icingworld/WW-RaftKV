#pragma once

#include <string>

#include <RaftRpcDispatcher.h>

namespace WW
{

/**
 * @brief Raft 服务端
*/
class RaftRpcServer
{
private:
    std::unique_ptr<RaftRpcDispatcher> _Dispatcher;

public:
    RaftRpcServer(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service = nullptr);

    ~RaftRpcServer() = default;

public:
    /**
     * @brief 注册服务
    */
    void registerService(google::protobuf::Service * _Service);

    /**
     * @brief 启动服务端
    */
    void start();
};

/**
 * @brief Raft 服务端
*/
class KVOperationServer
{
private:
    std::unique_ptr<KVOperationDispatcher> _Dispatcher;

public:
    KVOperationServer(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service = nullptr);

    ~KVOperationServer() = default;

public:
    /**
     * @brief 注册服务
    */
    void registerService(google::protobuf::Service * _Service);

    /**
     * @brief 启动服务端
    */
    void start();
};

} // namespace WW
