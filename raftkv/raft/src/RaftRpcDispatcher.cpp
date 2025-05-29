#include "RaftRpcDispatcher.h"

#include <RaftRpcSerialization.h>
#include <RaftRpcClosure.h>
#include <RaftLogger.h>

namespace WW
{

RaftRpcDispatcher::RaftRpcDispatcher(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Service_map()
    , _Event_loop(_Loop)
    , _Server(nullptr)
{
}

void RaftRpcDispatcher::registerService(google::protobuf::Service * _Service)
{
    DEBUG("register service");
    // 获取服务的元信息
    const google::protobuf::ServiceDescriptor * service_dsc = _Service->GetDescriptor();
    std::string service_name = service_dsc->name();
    int method_count = service_dsc->method_count();

    // 创建服务信息实例
    ServiceInfo info;

    DEBUG("== register begin ==");
    DEBUG("service name: %s", service_name.c_str());
    for (int i = 0; i < method_count; ++i) {
        // 依次读取方法信息，并存入表
        const google::protobuf::MethodDescriptor * method_dsc = service_dsc->method(i);
        info._Method_map[method_dsc->name()] = method_dsc;
        DEBUG("- method name: %s", method_dsc->name().c_str());
    }
    DEBUG("== register end ==");

    // 保存信息
    info._Service = _Service;
    _Service_map[service_name] = info;
}

void RaftRpcDispatcher::start()
{
    muduo::net::InetAddress address(_Ip, std::stoi(_Port));
    _Server = new muduo::net::TcpServer(_Event_loop, address, "RaftRpcDispatcher");

    // 设置回调函数
    _Server->setConnectionCallback(std::bind(&RaftRpcDispatcher::_OnConnection, this, std::placeholders::_1));
    _Server->setMessageCallback(std::bind(&RaftRpcDispatcher::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 启动
    DEBUG("server start at: %s:%d", _Ip.c_str(), std::stoi(_Port));
    _Server->start();
}

void RaftRpcDispatcher::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (!_Conn->connected()) {
        // 连接关闭，断开连接
        _Conn->shutdown();
    }
}

void RaftRpcDispatcher::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    // 反序列化提取信息
    std::string recv_buf = _Buffer->retrieveAllAsString();
    std::string service_name;
    std::string method_name;
    std::string arg_buf;

    if (!RaftRpcSerialization::deserializeHeader(recv_buf, service_name, method_name, arg_buf)) {
        return;
    }

    // 从表中查找服务和方法
    auto service_it = _Service_map.find(service_name);
    if (service_it == _Service_map.end()) {
        return;
    }

    const ServiceInfo & info = service_it->second;

    auto method_it = info._Method_map.find(method_name);
    if (method_it == info._Method_map.end()) {
        return;
    }

    // 创建请求结构
    std::shared_ptr<google::protobuf::Message> request = std::shared_ptr<google::protobuf::Message>
        (info._Service->GetRequestPrototype(method_it->second).New());

    // 反序列化请求
    if (!RaftRpcSerialization::deserializeArgs(arg_buf, request.get())) {
        return;
    }

    // 创建响应
    std::shared_ptr<google::protobuf::Message> response = std::shared_ptr<google::protobuf::Message>
        (info._Service->GetResponsePrototype(method_it->second).New());

    // 注册回调
    google::protobuf::Closure * done = new RaftLambdaClosure([=]() {
        this->_SendResponse(service_name, method_name, _Conn, response.get());
    });

    // 调用实现
    info._Service->CallMethod(method_it->second, nullptr, request.get(), response.get(), done);
}

void RaftRpcDispatcher::_SendResponse(const std::string & _Service_name, const std::string & _Method_name, const muduo::net::TcpConnectionPtr & _Conn, google::protobuf::Message * _Response)
{
    std::string response_str;
    if (RaftRpcSerialization::serialize(_Service_name, _Method_name, *_Response, response_str)) {
        // 序列化成功，发送响应
        _Conn->send(response_str);
    }
}

RaftOperationDispatcher::RaftOperationDispatcher(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Service_map()
    , _Event_loop(_Loop)
    , _Server(nullptr)
{
}

void RaftOperationDispatcher::registerService(google::protobuf::Service * _Service)
{
    DEBUG("register service");
    // 获取服务的元信息
    const google::protobuf::ServiceDescriptor * service_dsc = _Service->GetDescriptor();
    std::string service_name = service_dsc->name();
    int method_count = service_dsc->method_count();

    // 创建服务信息实例
    ServiceInfo info;

    DEBUG("== register begin ==");
    DEBUG("service name: %s", service_name.c_str());
    for (int i = 0; i < method_count; ++i) {
        // 依次读取方法信息，并存入表
        const google::protobuf::MethodDescriptor * method_dsc = service_dsc->method(i);
        info._Method_map[method_dsc->name()] = method_dsc;
        DEBUG("- method name: %s", method_dsc->name().c_str());
    }
    DEBUG("== register end ==");

    // 保存信息
    info._Service = _Service;
    _Service_map[service_name] = info;
}

void RaftOperationDispatcher::start()
{
    muduo::net::InetAddress address(_Ip, std::stoi(_Port) + 1);
    _Server = new muduo::net::TcpServer(_Event_loop, address, "RaftOperationDispatcher");

    // 设置回调函数
    _Server->setConnectionCallback(std::bind(&RaftOperationDispatcher::_OnConnection, this, std::placeholders::_1));
    _Server->setMessageCallback(std::bind(&RaftOperationDispatcher::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 启动
    DEBUG("operation server start at: %s:%d", _Ip.c_str(), std::stoi(_Port) + 1);
    _Server->start();
}

void RaftOperationDispatcher::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (!_Conn->connected()) {
        // 连接关闭，断开连接
        _Conn->shutdown();
    }
}

void RaftOperationDispatcher::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    // 反序列化提取信息
    std::string recv_buf = _Buffer->retrieveAllAsString();
    std::string service_name;
    std::string method_name;
    std::string arg_buf;

    if (!RaftRpcSerialization::deserializeHeader(recv_buf, service_name, method_name, arg_buf)) {
        return;
    }

    // 从表中查找服务和方法
    auto service_it = _Service_map.find(service_name);
    if (service_it == _Service_map.end()) {
        return;
    }

    const ServiceInfo & info = service_it->second;

    auto method_it = info._Method_map.find(method_name);
    if (method_it == info._Method_map.end()) {
        return;
    }

    // 创建请求结构
    std::shared_ptr<google::protobuf::Message> request = std::shared_ptr<google::protobuf::Message>
        (info._Service->GetRequestPrototype(method_it->second).New());

    // 反序列化请求
    if (!RaftRpcSerialization::deserializeArgs(arg_buf, request.get())) {
        return;
    }

    // 创建响应
    std::shared_ptr<google::protobuf::Message> response = std::shared_ptr<google::protobuf::Message>
        (info._Service->GetResponsePrototype(method_it->second).New());

    // 注册回调
    google::protobuf::Closure * done = new RaftLambdaClosure([=]() {
        this->_SendResponse(service_name, method_name, _Conn, response.get());
    });

    // 调用实现
    info._Service->CallMethod(method_it->second, nullptr, request.get(), response.get(), done);
}

void RaftOperationDispatcher::_SendResponse(const std::string & _Service_name, const std::string & _Method_name, const muduo::net::TcpConnectionPtr & _Conn, google::protobuf::Message * _Response)
{
    std::string response_str;
    if (RaftRpcSerialization::serialize(_Service_name, _Method_name, *_Response, response_str)) {
        // 序列化成功，发送响应
        _Conn->send(response_str);
    }

    // 关闭连接
    _Conn->shutdown();
}

} // namespace WW
