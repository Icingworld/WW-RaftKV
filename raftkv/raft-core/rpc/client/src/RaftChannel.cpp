#include "RaftChannel.h"

#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>

#include <RpcSerialization.h> 

namespace WW
{

RaftChannel::RaftChannel(const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Event_loop()
    , _Context()
{
}

void RaftChannel::CallMethod(const google::protobuf::MethodDescriptor * method,
                             google::protobuf::RpcController * controller,
                             const google::protobuf::Message * request,
                             google::protobuf::Message * response,
                             google::protobuf::Closure * done)
{
    // 获取服务和方法名
    std::string service_name = method->service()->name();
    std::string method_name = method->name();

    // 序列化请求
    std::string request_str;
    if (!RpcSerialization::serialize(service_name, method_name, *request, request_str)) {
        // 序列化失败
        return;
    }

    // 创建地址
    muduo::net::InetAddress server_addr(_Ip, std::stoi(_Port));

    // 创建一个 TCP 连接
    muduo::net::TcpClient client(&_Event_loop, server_addr, "RaftChannel");

    // 设置回调函数
    client.setConnectionCallback(std::bind(&RaftChannel::_OnConnection, this, std::placeholders::_1));
    client.setMessageCallback(std::bind(&RaftChannel::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 保存上下文
    _Context.method = method;
    _Context.controller = controller;
    _Context.request = request;
    _Context.response = response;
    _Context.done = done;

    // 连接
    client.connect();
    // 启动事件循环
    _Event_loop.loop();
}

void RaftChannel::_OnConnection(const muduo::net::TcpConnectionPtr & conn)
{
    if (conn->connected()) {
        // 连接上服务端，准备发送序列化请求
        std::string service_name = _Context.method->service()->name();
        std::string method_name = _Context.method->name();

        // 序列化请求
        std::string request_str;
        if (!RpcSerialization::serialize(service_name, method_name, *_Context.request, request_str)) {
            // 序列化失败
            conn->shutdown();
            _Event_loop.quit();
            return;
        }

        // 发送请求
        conn->send(request_str);
    }
}

void RaftChannel::_OnMessage(const muduo::net::TcpConnectionPtr & conn, muduo::net::Buffer * buffer, muduo::Timestamp receive_time)
{
    std::string recv_buf = buffer->retrieveAllAsString();
    if (!_Context.response->ParseFromString(recv_buf)) {
        // 解析失败
        conn->shutdown();
        _Event_loop.quit();
        return;
    }

    // 调用回调函数，通知业务层
    if (_Context.done != nullptr) {
        _Context.done->Run();
    }

    // 关闭连接
    conn->shutdown();
    _Event_loop.quit();
}

} // namespace WW
