#pragma once

#include <string>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>

/**
 * @brief RpcChannel
*/
class ClientChannel : public google::protobuf::RpcChannel
{
private:
    /**
     * @brief 调用方法的上下文
    */
    class CallMethodContext
    {
    public:
        const google::protobuf::MethodDescriptor * _Method;
        google::protobuf::RpcController * _Controller;
        const google::protobuf::Message * _Request;
        google::protobuf::Message * _Response;
        google::protobuf::Closure * _Done;
    };

private:
    std::string _Ip;        // 服务端 IP
    std::string _Port;      // 服务端 PORT

    muduo::net::EventLoop * _Event_loop;      // muduo 事件循环
    muduo::net::TcpClient * _Client;

public:
    ClientChannel(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port);

    ~ClientChannel() = default;

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
    void CallMethod(const google::protobuf::MethodDescriptor * _Method,
                    google::protobuf::RpcController * _Controller,
                    const google::protobuf::Message * _Request,
                    google::protobuf::Message * _Response,
                    google::protobuf::Closure * _Done) override;

    /**
     * @brief 连接服务端
    */
    void connect();

private:
    /**
     * @brief 连接事件回调函数
    */
    void _OnConnection(const muduo::net::TcpConnectionPtr & _Conn);

    /**
     * @brief 消息事件回调函数
    */
    void _OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time);
};
