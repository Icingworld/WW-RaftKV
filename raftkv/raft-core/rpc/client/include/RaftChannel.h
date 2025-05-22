#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <muduo/net/EventLoop.h>

namespace WW
{

/**
 * @brief Raft 通道
*/
class RaftChannel : public google::protobuf::RpcChannel
{
public:
    /**
     * @brief 调用方法的上下文
    */
    class CallMethodContext
    {
    public:
        const google::protobuf::MethodDescriptor * method;
        google::protobuf::RpcController * controller;
        const google::protobuf::Message * request;
        google::protobuf::Message * response;
        google::protobuf::Closure * done;
    };

private:
    std::string _Ip;
    std::string _Port;
    muduo::net::EventLoop _Event_loop;
    CallMethodContext _Context;

public:
    RaftChannel(const std::string & _Ip, const std::string & _Port);

public:
    /**
     * @brief 调用方法
     * @details 该方法是基类 RpcChannel 的纯虚函数，当在 proto 文件中声明 service 时，
     * 为 service 生成的成员函数最终都调用了 RpcChannel->CallMethod
     * @param method 指向要调用的方法的描述信息，可以从中获取方法名、所属服务名、请求/响应类型等信息
     * @param controller 用于传递和保存 RPC 状态，如错误信息、取消、超时等，可自己扩展
     * @param request 客户端传入的方法参数
     * @param response 用于存放服务端响应的数据，调用成功后需要将反序列化的结果填充进来
     * @param done 完成后的回调函数
     */
    void CallMethod(const google::protobuf::MethodDescriptor * method,
                    google::protobuf::RpcController * controller,
                    const google::protobuf::Message * request,
                    google::protobuf::Message * response,
                    google::protobuf::Closure * done) override;

private:
    void _OnConnection(const muduo::net::TcpConnectionPtr & conn);

    void _OnMessage(const muduo::net::TcpConnectionPtr & conn, muduo::net::Buffer * buffer, muduo::Timestamp receive_time);
};

} // namespace WW
