#include "RaftRpcServer.h"

namespace WW
{

RaftRpcServer::RaftRpcServer(const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    _Dispatcher = new RaftRpcDispatcher(_Ip, _Port);

    if (_Service != nullptr) {
        registerService(_Service);
    }
}

RaftRpcServer::~RaftRpcServer()
{
    delete _Dispatcher;
}

void RaftRpcServer::run()
{
    _Dispatcher->run();
}

void RaftRpcServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

RaftOperationServer::RaftOperationServer(const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    _Dispatcher = new RaftOperationDispatcher(_Ip, _Port);

    if (_Service != nullptr) {
        registerService(_Service);
    }
}

RaftOperationServer::~RaftOperationServer()
{
    delete _Dispatcher;
}

void RaftOperationServer::run()
{
    _Dispatcher->run();
}

void RaftOperationServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

} // namespace WW
