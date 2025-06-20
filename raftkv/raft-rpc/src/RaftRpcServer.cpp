#include "RaftRpcServer.h"

namespace WW
{

RaftRpcServer::RaftRpcServer(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Dispatcher(nullptr)
{
    // 初始化 Dispatcher
    _Dispatcher = std::make_unique<RaftRpcDispatcher>(_Event_loop, _Ip, _Port);
}

void RaftRpcServer::registerService(std::unique_ptr<google::protobuf::Service> _Service)
{
    _Dispatcher->registerService(std::move(_Service));
}

void RaftRpcServer::start()
{
    _Dispatcher->start();
}

KVOperationServer::KVOperationServer(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Dispatcher(nullptr)
{
    // 初始化 Dispatcher
    _Dispatcher = std::make_unique<KVOperationDispatcher>(_Event_loop, _Ip, _Port);
}

void KVOperationServer::registerService(std::unique_ptr<google::protobuf::Service> _Service)
{
    _Dispatcher->registerService(std::move(_Service));
}

void KVOperationServer::start()
{
    _Dispatcher->start();
}

} // namespace WW
