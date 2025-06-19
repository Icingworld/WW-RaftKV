#include "RaftRpcDispatcher.h"

#include <functional>

#include <RaftRpcCRC32.h>
#include <RaftRpcSerialization.h>
#include <RaftRpcFixedHeader.h>
#include <RaftRpcController.h>
#include <RaftRpcClosure.h>

#include <muduo/net/InetAddress.h>

namespace WW
{

RaftRpcDispatcher::RaftRpcDispatcher(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Service_map()
    , _Event_loop(_Event_loop)
    , _Server(nullptr)
    , _Logger(Logger::getSyncLogger("RaftRpc"))
{
    // 初始化服务端
    muduo::net::InetAddress server_addr(_Ip, std::stoi(_Port));
    _Server = std::unique_ptr<muduo::net::TcpServer>(
        new muduo::net::TcpServer(_Event_loop.get(), server_addr, "RaftRpcDispatcher")
    );

    // 绑定回调函数
    _Server->setConnectionCallback(
        std::bind(&RaftRpcDispatcher::_OnConnection, this, std::placeholders::_1)
    );

    _Server->setMessageCallback(
        std::bind(&RaftRpcDispatcher::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
    );

    // 设置服务端线程数
    _Server->setThreadNum(2);
}

void RaftRpcDispatcher::registerService(std::unique_ptr<google::protobuf::Service> _Service)
{
    // 获取服务的元信息
    const google::protobuf::ServiceDescriptor * service_dsc = _Service->GetDescriptor();
    std::string service_name = service_dsc->name();
    int method_count = service_dsc->method_count();

    // 创建服务信息实例
    ServiceInfo info;
    for (int i = 0; i < method_count; ++i) {
        // 依次读取方法信息，并存入表
        const google::protobuf::MethodDescriptor * method_dsc = service_dsc->method(i);
        info._Method_map[method_dsc->name()] = method_dsc;
    }

    // 保存信息
    info._Service = std::move(_Service);
    _Service_map[service_name] = std::move(info);
}

void RaftRpcDispatcher::start()
{
    if (_Server != nullptr) {
        _Server->start();
        _Logger.debug("rpc server start at: " + _Ip + ":" + _Port);
    }
}

void RaftRpcDispatcher::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (!_Conn->connected()) {
        // 连接关闭
    }
}

void RaftRpcDispatcher::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    while (_Buffer->readableBytes() >= sizeof(RaftRpcFixedHeader)) {
        RaftRpcFixedHeader header;
        // 取出固定头
        ::memcpy(&header, _Buffer->peek(), sizeof(header));

        // 转换为本机序
        header._Magic_number = be64toh(header._Magic_number);
        header._Total_length = be32toh(header._Total_length);
        CRC32Type received_checksum = be32toh(header._Header_checksum);

        // 验证魔数
        if (header._Magic_number != kMagicNumber) {
            // 严重错误，清空缓冲区
            _Buffer->retrieveAll();
            _Logger.error("header magic number not matched");
            return;
        }

        // 验证校验和
        CRC32Type crc = RaftRpcFixedHeader::calculateHeaderChecksum(header);
        if (crc != received_checksum) {
            // 校验和验证失败，跳过坏头部
            _Buffer->retrieve(sizeof(header));
            _Logger.error("header checksum not matched");
            return;
        }

        // 检查完整数据包
        if (_Buffer->readableBytes() < header._Total_length) {
            // 数据不完整，等待更多数据
            return;
        }

        _Buffer->retrieve(sizeof(RaftRpcFixedHeader));
        std::string packet = _Buffer->retrieveAsString(header._Total_length - sizeof(RaftRpcFixedHeader));

        // 反序列化
        std::string service_name;
        std::string method_name;
        SequenceType sequence_id;
        std::string payload;
        if (!RaftRpcSerialization::deserialize(
            packet,
            service_name,
            method_name,
            sequence_id,
            payload
        )) {
            _Logger.error("request deserialization failed");
            return;
        }

        // 根据服务-方法名找到描述符
        auto service_it = _Service_map.find(service_name);
        if (service_it == _Service_map.end()) {
            return;
        }

        const ServiceInfo & info = service_it->second;

        auto method_it = info._Method_map.find(method_name);
        if (method_it == info._Method_map.end()) {
            return;
        }

        // 根据方法描述符创建请求结构
        std::unique_ptr<google::protobuf::Message> request = std::unique_ptr<google::protobuf::Message>(
            info._Service->GetRequestPrototype(method_it->second).New()
        );

        // 反序列化 payload
        if (!request->ParseFromString(payload)) {
            _Logger.error("parse payload failed");
            return;
        }

        // 创建响应结构
        std::unique_ptr<google::protobuf::Message> response = std::unique_ptr<google::protobuf::Message>(
            info._Service->GetResponsePrototype(method_it->second).New()
        );

        // 创建控制器
        std::unique_ptr<RaftRpcController> controller = std::unique_ptr<RaftRpcController>(
            new RaftRpcController()
        );

        // 取出指针
        RaftRpcController * controller_ptr = controller.get();
        google::protobuf::Message * request_ptr = request.get();
        google::protobuf::Message * response_ptr = response.get();

        // 创建闭包
        RaftRpcServerClosure * done = new RaftRpcServerClosure(
            sequence_id, std::move(controller), std::move(request), std::move(response), [=]() {
            this->_SendResponse(_Conn, service_name, method_name, sequence_id, response_ptr, controller_ptr);
        });

        info._Service->CallMethod(method_it->second, controller_ptr, request_ptr, response_ptr, done);
    }
}

void RaftRpcDispatcher::_SendResponse(const muduo::net::TcpConnectionPtr & _Conn, const std::string & _Service_name,
                                    const std::string & _Method_name, SequenceType _Sequence_id,
                                    google::protobuf::Message * _Response, const google::protobuf::RpcController * _Controller)
{
    // 检查是否失败
    if (_Controller->Failed()) {
        return;
    }

    std::string rpc_str;
    if (!RaftRpcSerialization::serialize(_Service_name, _Method_name, _Sequence_id, *_Response, rpc_str)) {
        // 序列化失败
        _Logger.error("response serialization failed");
    }

    // 添加固定头
    RaftRpcFixedHeader header;
    header._Magic_number = 0x0A1B2C3D4E5F6A7B;
    header._Total_length = sizeof(RaftRpcFixedHeader) + rpc_str.size();
    // 计算校验和
    CRC32Type crc32 = RaftRpcFixedHeader::calculateHeaderChecksum(header);
    // 转换为网络序
    header._Magic_number = htobe64(header._Magic_number);
    header._Total_length = htobe32(header._Total_length);
    header._Header_checksum = htobe32(crc32);

    // 生成固定头字符串
    std::string header_str;
    header_str.append(reinterpret_cast<const char*>(&header), sizeof(header));
    std::string response_str = header_str + rpc_str;

    // 序列化成功，发送响应
    _Conn->send(response_str);
}

KVOperationDispatcher::KVOperationDispatcher(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Service_map()
    , _Event_loop(_Event_loop)
    , _Server(nullptr)
    , _Logger(Logger::getSyncLogger("RaftRpc"))
{
    // 初始化服务端
    muduo::net::InetAddress server_addr(_Ip, std::stoi(_Port));
    _Server = std::unique_ptr<muduo::net::TcpServer>(
        new muduo::net::TcpServer(_Event_loop.get(), server_addr, "KVOperationDispatcher")
    );

    // 绑定回调函数
    _Server->setConnectionCallback(
        std::bind(&KVOperationDispatcher::_OnConnection, this, std::placeholders::_1)
    );

    _Server->setMessageCallback(
        std::bind(&KVOperationDispatcher::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
    );

    // 设置线程数
    _Server->setThreadNum(2);
}

void KVOperationDispatcher::registerService(std::unique_ptr<google::protobuf::Service> _Service)
{
    // 获取服务的元信息
    const google::protobuf::ServiceDescriptor * service_dsc = _Service->GetDescriptor();
    std::string service_name = service_dsc->name();
    int method_count = service_dsc->method_count();

    // 创建服务信息实例
    ServiceInfo info;
    for (int i = 0; i < method_count; ++i) {
        // 依次读取方法信息，并存入表
        const google::protobuf::MethodDescriptor * method_dsc = service_dsc->method(i);
        info._Method_map[method_dsc->name()] = method_dsc;
    }

    // 保存信息
    info._Service = std::move(_Service);
    _Service_map[service_name] = std::move(info);
}

void KVOperationDispatcher::start()
{
    if (_Server != nullptr) {
        _Server->start();
        _Logger.debug("kv server start at: " + _Ip + ":" + _Port);
    }
}

void KVOperationDispatcher::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (!_Conn->connected()) {
        // 连接关闭
    }
}

void KVOperationDispatcher::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    while (_Buffer->readableBytes() >= sizeof(RaftRpcFixedHeader)) {
        RaftRpcFixedHeader header;
        // 取出固定头
        ::memcpy(&header, _Buffer->peek(), sizeof(header));

        // 转换为本机序
        header._Magic_number = be64toh(header._Magic_number);
        header._Total_length = be32toh(header._Total_length);
        CRC32Type received_checksum = be32toh(header._Header_checksum);

        // 验证魔数
        if (header._Magic_number != kMagicNumber) {
            // 严重错误，清空缓冲区
            _Buffer->retrieveAll();
            _Logger.error("header magic number not matched");
            return;
        }

        // 验证校验和
        CRC32Type crc = RaftRpcFixedHeader::calculateHeaderChecksum(header);
        if (crc != received_checksum) {
            // 校验和验证失败，跳过坏头部
            _Buffer->retrieve(sizeof(header));
            _Logger.error("header checksum not matched");
            return;
        }

        // 检查完整数据包
        if (_Buffer->readableBytes() < header._Total_length) {
            // 数据不完整，等待更多数据
            return;
        }

        _Buffer->retrieve(sizeof(RaftRpcFixedHeader));
        std::string packet = _Buffer->retrieveAsString(header._Total_length - sizeof(RaftRpcFixedHeader));

        // 反序列化
        std::string service_name;
        std::string method_name;
        SequenceType sequence_id;
        std::string payload;
        if (!RaftRpcSerialization::deserialize(
            packet,
            service_name,
            method_name,
            sequence_id,
            payload
        )) {
            _Logger.error("request deserialization failed");
            return;
        }

        // 根据服务-方法名找到描述符
        auto service_it = _Service_map.find(service_name);
        if (service_it == _Service_map.end()) {
            return;
        }

        const ServiceInfo & info = service_it->second;

        auto method_it = info._Method_map.find(method_name);
        if (method_it == info._Method_map.end()) {
            return;
        }

        // 根据方法描述符创建请求结构
        std::unique_ptr<google::protobuf::Message> request = std::unique_ptr<google::protobuf::Message>(
            info._Service->GetRequestPrototype(method_it->second).New()
        );

        // 反序列化 payload
        if (!request->ParseFromString(payload)) {
            _Logger.error("parse payload failed");
            return;
        }

        // 创建响应结构
        std::unique_ptr<google::protobuf::Message> response = std::unique_ptr<google::protobuf::Message>(
            info._Service->GetResponsePrototype(method_it->second).New()
        );

        // 创建控制器
        std::unique_ptr<RaftRpcController> controller = std::unique_ptr<RaftRpcController>(
            new RaftRpcController()
        );

        // 取出指针
        RaftRpcController * controller_ptr = controller.get();
        google::protobuf::Message * request_ptr = request.get();
        google::protobuf::Message * response_ptr = response.get();

        // 创建闭包
        RaftRpcServerClosure * done = new RaftRpcServerClosure(
            sequence_id, std::move(controller), std::move(request), std::move(response), [=]() {
            this->_SendResponse(_Conn, service_name, method_name, sequence_id, response_ptr, controller_ptr);
        });

        info._Service->CallMethod(method_it->second, controller_ptr, request_ptr, response_ptr, done);
    }
}

void KVOperationDispatcher::_SendResponse(const muduo::net::TcpConnectionPtr & _Conn, const std::string & _Service_name,
                                        const std::string & _Method_name, SequenceType _Sequence_id,
                                        google::protobuf::Message * _Response, const google::protobuf::RpcController * _Controller)
{
    // 检查是否失败
    if (_Controller->Failed()) {
        return;
    }

    std::string rpc_str;
    if (!RaftRpcSerialization::serialize(_Service_name, _Method_name, _Sequence_id, *_Response, rpc_str)) {
        // 序列化失败
        _Logger.error("response serialization failed");
    }

    // 添加固定头
    RaftRpcFixedHeader header;
    header._Magic_number = 0x0A1B2C3D4E5F6A7B;
    header._Total_length = sizeof(RaftRpcFixedHeader) + rpc_str.size();
    // 计算校验和
    CRC32Type crc32 = RaftRpcFixedHeader::calculateHeaderChecksum(header);
    // 转换为网络序
    header._Magic_number = htobe64(header._Magic_number);
    header._Total_length = htobe32(header._Total_length);
    header._Header_checksum = htobe32(crc32);

    // 生成固定头字符串
    std::string header_str;
    header_str.append(reinterpret_cast<const char*>(&header), sizeof(header));
    std::string response_str = header_str + rpc_str;

    // 序列化成功，发送响应
    _Conn->send(response_str);

    // 采用短链接
    _Conn->shutdown();
}

} // namespace WW
