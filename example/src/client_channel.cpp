#include "client_channel.h"

#include <iostream>

#include <muduo/net/TcpConnection.h>
#include <muduo/net/InetAddress.h>

#include <RaftRpcSerialization.h>

ClientChannel::ClientChannel(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Event_loop(_Loop)
{
}

void ClientChannel::CallMethod(const google::protobuf::MethodDescriptor * _Method,
                                google::protobuf::RpcController * _Controller,
                                const google::protobuf::Message * _Request,
                                google::protobuf::Message * _Response,
                                google::protobuf::Closure * _Done)
{
    // 获取服务和方法名
    std::string service_name = _Method->service()->name();
    std::string method_name = _Method->name();

    // 序列化请求
    std::string request_str;
    if (!WW::RaftRpcSerialization::serialize(service_name, method_name, *_Request, request_str)) {
        // 序列化失败
        return;
    }

    muduo::net::TcpConnectionPtr conn = _Client->connection();

    // 保存上下文
    CallMethodContext context;
    context._Method = _Method;
    context._Controller = _Controller;
    context._Request = _Request;
    context._Response = _Response;
    context._Done = _Done;
    conn->setContext(context);
    
    // 发送请求
    conn->send(request_str);
}

void ClientChannel::connect()
{
    // 创建地址
    muduo::net::InetAddress server_addr(_Ip, std::stoi(_Port));

    // 创建一个 TCP 连接
    _Client = new muduo::net::TcpClient(_Event_loop, server_addr, "ClientChannel");

    // 设置回调函数
    _Client->setConnectionCallback(std::bind(&ClientChannel::_OnConnection, this, std::placeholders::_1));
    _Client->setMessageCallback(std::bind(&ClientChannel::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 连接
    _Client->connect();
}

void ClientChannel::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (_Conn->connected()) {
        // std::cout << "connected" << std::endl;
    } else {
        std::cout << "disconnected" << std::endl;
    }
}

void ClientChannel::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    CallMethodContext context = boost::any_cast<CallMethodContext>(_Conn->getContext());

    // 获取接收到的字符串
    std::string recv_buf = _Buffer->retrieveAllAsString();
    
    // 提取出 Args 部分
    std::string service_name;
    std::string method_name;
    std::string args_str;
    if (!WW::RaftRpcSerialization::deserializeHeader(recv_buf, service_name, method_name, args_str)) {
        // 解析失败
        goto SHUTDOWN;
    }

    // 解析为 Message
    if (!WW::RaftRpcSerialization::deserializeArgs(args_str, context._Response)) {
        // 解析失败
        goto SHUTDOWN;
    }

    // 调用回调函数，通知业务层
    context._Done->Run();

    // 关闭连接
SHUTDOWN:
    _Conn->shutdown();
}
