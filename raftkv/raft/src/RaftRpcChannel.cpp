#include "RaftRpcChannel.h"

#include <muduo/net/TcpConnection.h>
#include <muduo/net/InetAddress.h>

#include <RaftRpcSerialization.h>
#include <RaftLogger.h>

namespace WW
{

RaftRpcChannel::RaftRpcChannel(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Event_loop(_Loop)
    , _Context()
{
}

void RaftRpcChannel::CallMethod(const google::protobuf::MethodDescriptor * _Method,
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
    if (!RaftRpcSerialization::serialize(service_name, method_name, *_Request, request_str)) {
        // 序列化失败
        return;
    }

    muduo::net::TcpConnectionPtr conn = _Client->connection();

    // 设置上下文
    _Context._Method = _Method;
    _Context._Request = _Request;
    _Context._Response = _Response;
    _Context._Done = _Done;
    conn->setContext(_Context);

    conn->send(request_str);
}

void RaftRpcChannel::connect()
{
    // 创建地址
    muduo::net::InetAddress server_addr(_Ip, std::stoi(_Port));

    // 创建一个 TCP 连接
    _Client = new muduo::net::TcpClient(_Event_loop, server_addr, "RaftRpcChannel");

    // 设置回调函数
    _Client->setConnectionCallback(std::bind(&RaftRpcChannel::_OnConnection, this, std::placeholders::_1));
    _Client->setMessageCallback(std::bind(&RaftRpcChannel::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 连接
    _Client->connect();
}

void RaftRpcChannel::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (_Conn->connected()) {
        DEBUG("connected");
    } else {
        ERROR("disconnected");
    }
}

void RaftRpcChannel::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    CallMethodContext context = boost::any_cast<CallMethodContext>(_Conn->getContext());

    // 获取接收到的字符串
    std::string recv_buf = _Buffer->retrieveAllAsString();
    
    // 提取出 Args 部分
    std::string service_name;
    std::string method_name;
    std::string args_str;
    if (!RaftRpcSerialization::deserializeHeader(recv_buf, service_name, method_name, args_str)) {
        // 解析失败
        return;
    }

    // 解析为 Message
    if (!RaftRpcSerialization::deserializeArgs(args_str, context._Response)) {
        // 解析失败
        return;
    }

    // 调用回调函数，通知业务层
    context._Done->Run();
}

} // namespace WW
