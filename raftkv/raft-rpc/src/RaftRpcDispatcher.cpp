#include "RaftRpcDispatcher.h"

#include <functional>

#include <RaftRpcCRC32.h>
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
}

void RaftRpcDispatcher::registerService(google::protobuf::Service * _Service)
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
    info._Service = _Service;
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
        // 连接关闭，断开连接
        _Conn->shutdown();
    }
}

void RaftRpcDispatcher::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    while (_Buffer->readableBytes() >= sizeof(FixedHeader)) {
        FixedHeader header;
        // 取出固定头
        ::memcpy(&header, _Buffer->peek(), sizeof(header));

        // 转换为本机序
        header.magic_number = be64toh(header.magic_number);
        header.total_length = be32toh(header.total_length);
        uint32_t received_checksum = be32toh(header.header_checksum);

        // 验证魔数
        if (header.magic_number != 0x0A1B2C3D4E5F6A7B) {
            // 严重错误，清空缓冲区
            printf("magic_number: 0x%016lx\n", header.magic_number);
            _Buffer->retrieveAll();
            _Logger.error("header magic number not matched");
            return;
        }

        // 验证校验和
        uint32_t crc = RaftRpcSerialization::_CalculateHeaderChecksum(header);
        if (crc != received_checksum) {
            // 校验和验证失败，跳过坏头部
            _Buffer->retrieve(sizeof(header));
            _Logger.error("header checksum not matched");
            return;
        }
        
        // 检查完整数据包
        if (_Buffer->readableBytes() < header.total_length) {
            // 数据不完整，等待更多数据
            return;
        }

        _Buffer->retrieve(sizeof(FixedHeader));
        std::string packet = _Buffer->retrieveAsString(header.total_length - sizeof(FixedHeader));

        // 反序列化
        std::string service_name;
        std::string method_name;
        uint64_t sequence_id;
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
        google::protobuf::Message * request = info._Service->GetRequestPrototype(method_it->second).New();

        // 反序列化 payload
        if (!request->ParseFromString(payload)) {
            _Logger.error("parse payload failed");
            return;
        }

        // 创建响应结构
        google::protobuf::Message * response = info._Service->GetResponsePrototype(method_it->second).New();

        // 创建 Controller
        RaftRpcController * controller = new RaftRpcController();

        // 创建回调函数
        google::protobuf::Closure * done = new RaftRpcServerClosure(sequence_id, response, [=]() {
            this->_SendResponse(_Conn, service_name, method_name, sequence_id, response);
        });

        info._Service->CallMethod(method_it->second, controller, request, nullptr, done);
    }
}

void RaftRpcDispatcher::_SendResponse(const muduo::net::TcpConnectionPtr & _Conn, const std::string & _Service_name,
    const std::string & _Method_name, uint64_t _Sequence_id, google::protobuf::Message * _Response)
{
    std::string rpc_str;
    if (!RaftRpcSerialization::serialize(_Service_name, _Method_name, _Sequence_id, *_Response, rpc_str)) {
        // 序列化失败
        _Logger.error("response serialization failed");
    }

    // 添加固定头
    FixedHeader header;
    header.magic_number = 0x0A1B2C3D4E5F6A7B;
    header.total_length = sizeof(FixedHeader) + rpc_str.size();
    // 计算校验和
    uint32_t crc32 = RaftRpcSerialization::_CalculateHeaderChecksum(header);
    // 转换为网络序
    header.magic_number = htobe64(header.magic_number);
    header.total_length = htobe32(header.total_length);
    header.header_checksum = htobe32(crc32);

    // 生成固定头字符串
    std::string header_str;
    header_str.append(reinterpret_cast<const char*>(&header), sizeof(header));
    std::string response_str = header_str + rpc_str;

    // 序列化成功，发送响应
    // _Logger.debug("send response");
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
}

void KVOperationDispatcher::registerService(google::protobuf::Service * _Service)
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
    info._Service = _Service;
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
        // 连接关闭，断开连接
        _Conn->shutdown();
    }
}

void KVOperationDispatcher::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    while (_Buffer->readableBytes() >= sizeof(FixedHeader)) {
        FixedHeader header;
        // 取出固定头
        ::memcpy(&header, _Buffer->peek(), sizeof(header));

        // 转换为本机序
        header.magic_number = be64toh(header.magic_number);
        header.total_length = be32toh(header.total_length);
        uint32_t received_checksum = be32toh(header.header_checksum);

        // 验证魔数
        if (header.magic_number != 0x0A1B2C3D4E5F6A7B) {
            // 严重错误，清空缓冲区
            printf("magic_number: 0x%016lx\n", header.magic_number);
            _Buffer->retrieveAll();
            _Logger.error("header magic number not matched");
            return;
        }

        // 验证校验和
        uint32_t crc = RaftRpcSerialization::_CalculateHeaderChecksum(header);
        if (crc != received_checksum) {
            // 校验和验证失败，跳过坏头部
            _Buffer->retrieve(sizeof(header));
            _Logger.error("header checksum not matched");
            return;
        }
        
        // 检查完整数据包
        if (_Buffer->readableBytes() < header.total_length) {
            // 数据不完整，等待更多数据
            return;
        }

        _Buffer->retrieve(sizeof(FixedHeader));
        std::string packet = _Buffer->retrieveAsString(header.total_length - sizeof(FixedHeader));

        // 反序列化
        std::string service_name;
        std::string method_name;
        uint64_t sequence_id;
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
        google::protobuf::Message * request = info._Service->GetRequestPrototype(method_it->second).New();

        // 反序列化 payload
        if (!request->ParseFromString(payload)) {
            _Logger.error("parse payload failed");
            return;
        }

        // 创建响应结构
        google::protobuf::Message * response = info._Service->GetResponsePrototype(method_it->second).New();

        // 创建 Controller
        RaftRpcController * controller = new RaftRpcController();

        // 创建回调函数
        google::protobuf::Closure * done = new RaftRpcServerClosure(sequence_id, response, [=]() {
            this->_SendResponse(_Conn, service_name, method_name, sequence_id, response);
        });

        info._Service->CallMethod(method_it->second, controller, request, nullptr, done);
    }
}

void KVOperationDispatcher::_SendResponse(const muduo::net::TcpConnectionPtr & _Conn, const std::string & _Service_name,
    const std::string & _Method_name, uint64_t _Sequence_id, google::protobuf::Message * _Response)
{
    std::string rpc_str;
    if (!RaftRpcSerialization::serialize(_Service_name, _Method_name, _Sequence_id, *_Response, rpc_str)) {
        // 序列化失败
        _Logger.error("response serialization failed");
    }

    // 添加固定头
    FixedHeader header;
    header.magic_number = 0x0A1B2C3D4E5F6A7B;
    header.total_length = sizeof(FixedHeader) + rpc_str.size();
    // 计算校验和
    uint32_t crc32 = RaftRpcSerialization::_CalculateHeaderChecksum(header);
    // 转换为网络序
    header.magic_number = htobe64(header.magic_number);
    header.total_length = htobe32(header.total_length);
    header.header_checksum = htobe32(crc32);

    // 生成固定头字符串
    std::string header_str;
    header_str.append(reinterpret_cast<const char*>(&header), sizeof(header));
    std::string response_str = header_str + rpc_str;

    // 序列化成功，发送响应
    // _Logger.debug("send response");
    _Conn->send(response_str);

    // 采用短链接
    _Conn->shutdown();
}

} // namespace WW
