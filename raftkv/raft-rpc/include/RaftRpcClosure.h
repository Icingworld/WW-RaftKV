#pragma once

#include <string>
#include <functional>

#include <RaftRpcCommon.h>
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/service.h>

namespace WW
{

/**
 * @brief RaftRpcClosure
*/
template <typename RequestType, typename ResponseType>
class RaftRpcClientClosure : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void(const ResponseType *, google::protobuf::RpcController *)>;

private:
    google::protobuf::RpcController * _Controller;
    const RequestType * _Request;
    ResponseType * _Response;
    ResponseCallback _Callback;

public:
    RaftRpcClientClosure(google::protobuf::RpcController * _Controller, const RequestType * _Request, ResponseType * _Response, ResponseCallback _Callback)
        : _Controller(_Controller)
        , _Request(_Request)
        , _Response(_Response)
        , _Callback(_Callback)
    {
    }

    ~RaftRpcClientClosure()
    {
        delete _Controller;
        delete _Request;
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
 * @brief RaftRpcServerClosure
*/
class RaftRpcServerClosure : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void()>;

private:
    SequenceType _Sequence_id;
    google::protobuf::Message * _Response;
    ResponseCallback _Callback;

public:
    RaftRpcServerClosure(SequenceType _Sequence_id, google::protobuf::Message * _Response, ResponseCallback _Callback);

    ~RaftRpcServerClosure();

public:
    void Run() override;

    /**
     * @brief 获取闭包中储存的响应
     * @return `google::protobuf::Message *`
    */
    google::protobuf::Message * response();

    /**
     * @brief 获取闭包中储存的请求序列号
     * @return 序列号
    */
    SequenceType sequenceId() const;
};

} // namespace WW