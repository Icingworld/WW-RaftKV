#include "RaftServer.h"

namespace WW
{

// 调用顺序：构造Raft -> 传入ServiceImpl -> 传入Server

RaftServer::RaftServer(const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service)
    : _Dispatcher(nullptr)
{
    _Dispatcher = new RaftDispatcher(_Ip, _Port);
}

RaftServer::~RaftServer()
{
    delete _Dispatcher;
}

void RaftServer::run()
{
    _Dispatcher->run();
}

void RaftServer::registerService(google::protobuf::Service * _Service)
{
    _Dispatcher->registerService(_Service);
}

} // namespace WW
