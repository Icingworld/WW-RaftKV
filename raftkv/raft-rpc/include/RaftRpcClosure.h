#pragma once

#include <memory>
#include <string>
#include <functional>

#include <RaftRpcCommon.h>

#include <muduo/net/TcpConnection.h>

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>

namespace WW
{

/**
 * @brief RaftRpcClosure
 * @details 拥有控制器、请求和响应的所有权
*/
template <typename RequestType, typename ResponseType>
class RaftRpcClientClosure : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void(const ResponseType *, const google::protobuf::RpcController *)>;

private:
    std::unique_ptr<google::protobuf::RpcController> _Controller;   // 控制器
    std::unique_ptr<RequestType> _Request;                          // 请求
    std::unique_ptr<ResponseType> _Response;                        // 响应
    ResponseCallback _Callback;

public:
    RaftRpcClientClosure(std::unique_ptr<google::protobuf::RpcController> _Controller,
                        std::unique_ptr<RequestType> _Request,
                        std::unique_ptr<ResponseType> _Response,
                        ResponseCallback && _Callback)
        : _Controller(std::move(_Controller))
        , _Request(std::move(_Request))
        , _Response(std::move(_Response))
        , _Callback(std::forward<ResponseCallback>(_Callback))
    {
    }

    ~RaftRpcClientClosure()
    {
    }

public:
    void Run() override
    {
        // 调用回调函数
        _Callback(_Response.get(), _Controller.get());

        // 析构闭包
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