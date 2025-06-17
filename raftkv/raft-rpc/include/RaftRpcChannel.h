#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <map>
#include <mutex>

#include <Logger.h>
#include <RaftRpcSerialization.h>
#include <RaftRpcCommon.h>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <muduo/net/InetAddress.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TimerId.h>

namespace WW
{

/**
 * @brief RpcChannel
*/
class RaftRpcChannel : public google::protobuf::RpcChannel
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
        muduo::net::TimerId _Timer_id;
    };

private:
    std::string _Ip;
    std::string _Port;
    muduo::net::InetAddress _Server_addr;                       // 服务端地址
    muduo::net::EventLoop * _Event_loop;                        // muduo 事件循环
    std::unique_ptr<muduo::net::TcpClient> _Client;             // tcp 客户端

    std::mutex _Mutex;                                          // 表锁
    std::atomic<SequenceType> _Sequence_id;                     // 请求序列号
    std::map<SequenceType, CallMethodContext> _Pending_requests;// 排队中的请求表

    Logger & _Logger;

public:
    RaftRpcChannel(muduo::net::EventLoop * _Event_loop, const std::string & _Ip, const std::string & _Port);

    ~RaftRpcChannel();

public:
    /**
     * @brief 调用方法
     * @details 该方法是基类 RpcChannel 的纯虚函数，当在 proto 文件中声明 service 时，
     *          为 service 生成的成员函数最终都调用了 RpcChannel->CallMethod
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

    /**
     * @brief 断开连接
    */
    void disconnect();

private:
    /**
     * @brief 连接事件回调函数
    */
    void _OnConnection(const muduo::net::TcpConnectionPtr & _Conn);

    /**
     * @brief 消息事件回调函数
    */
    void _OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time);

    /**
     * @brief 超时处理
     * @param _Sequence_id 请求序列号
    */
    void _HandleTimeout(SequenceType _Sequence_id);
};

}