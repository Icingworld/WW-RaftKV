#include "RaftRpcServer.h"

namespace WW
{

RaftRpcServer::RaftRpcServer(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    // 初始化 Dispatcher
    _Dispatcher = std::unique_ptr<RaftRpcDispatcher>(
        new RaftRpcDispatcher(_Event_loop, _Ip, _Port)
    );

    if (_Service != nullptr) {
        registerService(_Service);
    }
}

void RaftRpcServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

void RaftRpcServer::start()
{
    _Dispatcher->start();
}

KVOperationServer::KVOperationServer(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    // 初始化 Dispatcher
    _Dispatcher = std::unique_ptr<KVOperationDispatcher>(
        new KVOperationDispatcher(_Event_loop, _Ip, _Port)
    );

    if (_Service != nullptr) {
        registerService(_Service);
    }
}

void KVOperationServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

void KVOperationServer::start()
{
    _Dispatcher->start();
}

} // namespace WW
