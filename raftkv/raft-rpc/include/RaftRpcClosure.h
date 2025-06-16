#pragma once

#include <string>
#include <functional>

#include <muduo/net/TcpConnection.h>
#include <google/protobuf/service.h>

namespace WW
{

/**
 * @brief RaftRpcClosure1
*/
template <typename ResponseType>
class RaftRpcClientClosure1 : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void(const ResponseType *, google::protobuf::RpcController *)>;

private:
    google::protobuf::RpcController * _Controller;
    ResponseType * _Response;
    ResponseCallback _Callback;

public:
    RaftRpcClientClosure1(google::protobuf::RpcController * _Controller, ResponseType * _Response, ResponseCallback _Callback)
        : _Controller(_Controller)
        , _Response(_Response)
        , _Callback(_Callback)
    {
    }

    ~RaftRpcClientClosure1()
    {
        delete _Controller;
        delete _Response;
    }

public:
    void Run() override
    {
        _Callback(_Response, _Controller);

        delete this;
    }
};

/**
 * @brief RaftRpcClosure2
*/
template <typename ResponseType>
class RaftRpcClientClosure2 : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void(int, const ResponseType *, google::protobuf::RpcController *)>;

private:
    int _Id;
    google::protobuf::RpcController * _Controller;
    ResponseType * _Response;
    ResponseCallback _Callback;

public:
    RaftRpcClientClosure2(int _Id, google::protobuf::RpcController * _Controller, ResponseType * _Response, ResponseCallback _Callback)
        : _Id(_Id)
        , _Controller(_Controller)
        , _Response(_Response)
        , _Callback(_Callback)
    {
    }

    ~RaftRpcClientClosure2()
    {
        delete _Controller;
        delete _Response;
    }

public:
    void Run() override
    {
        _Callback(_Id, _Response, _Controller);

        delete this;
    }
};

/**
 * @brief RaftRpcServerClosure
*/
class RaftRpcServerClosure : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void()>;

private:
    uint64_t _Sequence_id;
    google::protobuf::Message * _Response;
    ResponseCallback _Callback;

public:
    RaftRpcServerClosure(
            uint64_t _Sequence_id,
            google::protobuf::Message * _Response,
            ResponseCallback _Callback
    )
        : _Sequence_id(_Sequence_id)
        , _Response(_Response)
        , _Callback(std::move(_Callback))
    {
    }

    ~RaftRpcServerClosure()
    {
        delete _Response;
    }

public:
    void Run() override
    {
        _Callback();

        delete this;
    }

    google::protobuf::Message * response() { return _Response; }

    uint64_t sequence_id() const { return _Sequence_id; }
};

} // namespace WW