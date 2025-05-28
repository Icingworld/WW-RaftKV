#include "client_channel.h"

#include <muduo/net/TcpConnection.h>
#include <muduo/net/InetAddress.h>

#include <RaftRpcSerialization.h>

ClientChannel::ClientChannel(const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Event_loop()
    , _Context()
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

    // 创建地址
    muduo::net::InetAddress server_addr(_Ip, std::stoi(_Port));

    // 创建一个 TCP 连接
    muduo::net::TcpClient * client = new muduo::net::TcpClient(&_Event_loop, server_addr, "ClientChannel");

    // 设置回调函数
    client->setConnectionCallback(std::bind(&ClientChannel::_OnConnection, this, std::placeholders::_1));
    client->setMessageCallback(std::bind(&ClientChannel::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 保存上下文
    _Context._Method = _Method;
    _Context._Controller = _Controller;
    _Context._Request = _Request;
    _Context._Response = _Response;
    _Context._Done = _Done;
    _Context._Client = client;

    // 连接
    client->connect();
    // 启动事件循环
    _Event_loop.loop();
}

void ClientChannel::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (_Conn->connected()) {
        // 连接上服务端，准备发送序列化请求
        std::string service_name = _Context._Method->service()->name();
        std::string method_name = _Context._Method->name();

        // 序列化请求
        std::string request_str;
        if (!WW::RaftRpcSerialization::serialize(service_name, method_name, *_Context._Request, request_str)) {
            // 序列化失败
            _Conn->shutdown();
            _Event_loop.quit();
            // delete _Context._Client;
            return;
        }

        // 发送请求
        _Conn->send(request_str);
    }
}

void ClientChannel::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    // 获取接收到的字符串
    std::string recv_buf = _Buffer->retrieveAllAsString();
    
    // 提取出 Args 部分
    std::string service_name;
    std::string method_name;
    std::string args_str;
    if (!WW::RaftRpcSerialization::deserializeHeader(recv_buf, service_name, method_name, args_str)) {
        // 解析失败
        // 关闭连接
        _Conn->shutdown();
        _Event_loop.quit();
        // delete _Context._Client;
        return;
    }

    // 解析为 Message
    if (!WW::RaftRpcSerialization::deserializeArgs(args_str, _Context._Response)) {
        // 解析失败
        // 关闭连接
        _Conn->shutdown();
        _Event_loop.quit();
        // delete _Context._Client;
        return;
    }

    // 调用回调函数，通知业务层
    _Context._Done->Run();

    // 关闭连接
    _Conn->shutdown();
    _Event_loop.quit();
    // delete _Context._Client;
}
