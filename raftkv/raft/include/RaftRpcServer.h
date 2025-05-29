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
    RaftRpcDispatcher * _Dispatcher;

public:
    RaftRpcServer(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service = nullptr);

    ~RaftRpcServer();

public:
    /**
     * @brief 启动服务端
    */
    void start();

    /**
     * @brief 注册服务
    */
    void registerService(google::protobuf::Service * _Service);
};

/**
 * @brief Raft 服务端
*/
class RaftOperationServer
{
private:
    RaftOperationDispatcher * _Dispatcher;

public:
    RaftOperationServer(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service = nullptr);

    ~RaftOperationServer();

public:
    /**
     * @brief 启动服务端
    */
    void start();

    /**
     * @brief 注册服务
    */
    void registerService(google::protobuf::Service * _Service);
};

} // namespace WW
