#pragma once

#include <memory>
#include <string>
#include <functional>

#include <Memory.h>
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
template <typename ControllerType, typename RequestType, typename ResponseType>
class RaftRpcClientClosure : public google::protobuf::Closure
{
public:
    using ResponseCallback = std::function<void(const ResponseType *, const ControllerType *)>;

    using ControllerDeleterFunc = MemoryPoolDeleter<ControllerType>;
    using RequestTypeDeleterFunc = MemoryPoolDeleter<RequestType>;
    using ResponseTypeDeleterFunc = MemoryPoolDeleter<ResponseType>;

private:
    std::unique_ptr<ControllerType, ControllerDeleterFunc> _Controller;    // 控制器
    std::unique_ptr<RequestType, RequestTypeDeleterFunc> _Request;                          // 请求
    std::unique_ptr<ResponseType, ResponseTypeDeleterFunc> _Response;                       // 响应
    ResponseCallback _Callback;

public:
    RaftRpcClientClosure(std::unique_ptr<ControllerType, ControllerDeleterFunc> _Controller,
                        std::unique_ptr<RequestType, RequestTypeDeleterFunc> _Request,
                        std::unique_ptr<ResponseType, ResponseTypeDeleterFunc> _Response,
                        ResponseCallback && _Callback)
        : _Controller(std::move(_Controller))
        , _Request(std::move(_Request))
        , _Response(std::move(_Response))
        , _Callback(std::move(_Callback))
    {
    }

    ~RaftRpcClientClosure() = default;

public:
    void Run() override
    {
        // 调用回调函数
        _Callback(_Response.get(), _Controller.get());

        // 析构闭包
        this->~RaftRpcClientClosure();
        // 释放内存
        thread_local MemoryPoolAllocator<RaftRpcClientClosure> allocator;
        allocator.deallocate(this, 1);
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
    std::unique_ptr<google::protobuf::RpcController> _Controller;
    std::unique_ptr<google::protobuf::Message> _Request;
    std::unique_ptr<google::protobuf::Message> _Response;
    ResponseCallback _Callback;

public:
    RaftRpcServerClosure(SequenceType _Sequence_id,
                        std::unique_ptr<google::protobuf::RpcController> _Controller,
                        std::unique_ptr<google::protobuf::Message> _Request,
                        std::unique_ptr<google::protobuf::Message> _Response,
                        ResponseCallback && _Callback);

    ~RaftRpcServerClosure() = default;

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