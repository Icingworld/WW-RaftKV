#include "RaftRpcServer.h"

namespace WW
{

RaftRpcServer::RaftRpcServer(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    _Dispatcher = new RaftRpcDispatcher(_Loop, _Ip, _Port);

    if (_Service != nullptr) {
        registerService(_Service);
    }
}

RaftRpcServer::~RaftRpcServer()
{
    delete _Dispatcher;
}

void RaftRpcServer::start()
{
    _Dispatcher->start();
}

void RaftRpcServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

RaftOperationServer::RaftOperationServer(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    _Dispatcher = new RaftOperationDispatcher(_Loop, _Ip, _Port);

    if (_Service != nullptr) {
        registerService(_Service);
    }
}

RaftOperationServer::~RaftOperationServer()
{
    delete _Dispatcher;
}

void RaftOperationServer::start()
{
    _Dispatcher->start();
}

void RaftOperationServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

} // namespace WW
