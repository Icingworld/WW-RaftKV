#include "RaftRpcChannel.h"

#include <functional>
#include <sstream>

#include <RaftRpcFixedHeader.h>
#include <RaftRpcCRC32.h>

#include <muduo/net/TcpConnection.h>

namespace WW
{

RaftRpcChannel::RaftRpcChannel(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Ip(_Ip)
    , _Port(_Port)
    , _Server_addr(_Ip, std::stoi(_Port))
    , _Event_loop(_Event_loop)
    , _Client(nullptr)
    , _Mutex()
    , _Sequence_id(1)
    , _Pending_requests()
    , _Logger(Logger::getSyncLogger("RaftRpc"))
{
    // 初始化客户端
    _Client = std::make_unique<muduo::net::TcpClient>(_Event_loop.get(), _Server_addr, "RaftRpcChannel");

    // 绑定回调函数
    _Client->setConnectionCallback(
        std::bind(&RaftRpcChannel::_OnConnection, this, std::placeholders::_1)
    );

    _Client->setMessageCallback(
        std::bind(&RaftRpcChannel::_OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
    );

    // 设置自动重连
    _Client->enableRetry();
}

RaftRpcChannel::~RaftRpcChannel()
{
    disconnect();
}

void RaftRpcChannel::CallMethod(const google::protobuf::MethodDescriptor * _Method,
                                google::protobuf::RpcController * _Controller,
                                const google::protobuf::Message * _Request,
                                google::protobuf::Message * _Response,
                                google::protobuf::Closure * _Done)
{
    // 生成序列号
    SequenceType sequence_id = _Sequence_id.fetch_add(1, std::memory_order_relaxed);

    // 构造上下文
    CallMethodContext context {
        ._Method = _Method,
        ._Controller = _Controller,
        ._Request = _Request,
        ._Response = _Response,
        ._Done = _Done,
        ._Timer_id = muduo::net::TimerId()
    };

    // 序列化请求
    std::string rpc_str;
    if (!RaftRpcSerialization::serialize(
        _Method->service()->name(),
        _Method->name(),
        sequence_id,
        *_Request,
        rpc_str
    )) {
        // 序列化失败，标记失败
        _Logger.error("request serialization failed");
        _Controller->SetFailed("request serialization failed");
        _Done->Run();
        return;
    }

    // 构造一个固定头
    RaftRpcFixedHeader header;
    header._Magic_number = kMagicNumber;
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
    std::string request_str = header_str + rpc_str;

    // 获取连接
    muduo::net::TcpConnectionPtr conn = _Client->connection();

    if (conn != nullptr && conn->connected()) {
        // 连接就绪
        // 绑定超时时间 5s 和回调函数
        context._Timer_id = _Event_loop->runAfter(
            5.0,
            std::bind(&RaftRpcChannel::_HandleTimeout, this, sequence_id)
        );

        // 注册到表中
        {
            std::lock_guard<std::mutex> lock(_Mutex);
            _Pending_requests[sequence_id] = std::move(context);
        }

        // 发送数据
        conn->send(request_str);
    } else {
        // 连接未就绪，标记失败
        _Logger.error("tcp connection not availavble");
        _Controller->SetFailed("tcp connection not availavble");
        _Done->Run();
    }
}

void RaftRpcChannel::connect()
{
    // 连接服务端
    if (_Client != nullptr) {
        _Client->connect();
    }
}

void RaftRpcChannel::disconnect()
{
    if (_Client != nullptr && _Client->connection()) {
        _Client->disconnect();
    }
}

void RaftRpcChannel::_OnConnection(const muduo::net::TcpConnectionPtr & _Conn)
{
    if (_Conn->connected()) {
        // 连接建立
        _Logger.debug("connected to: " + _Ip + ":" + _Port);
    } else {
        _Logger.debug("connection to: " + _Ip + ":" + _Port + " closed");
        // 连接断开，清理所有等待中的请求
        std::lock_guard<std::mutex> lock(_Mutex);
        for (std::pair<const SequenceType, CallMethodContext> & pair : _Pending_requests) {
            CallMethodContext & context = pair.second;
            // 清除定时器
            _Event_loop->cancel(context._Timer_id);

            // 标记失败
            context._Controller->SetFailed("connection already closed");
            context._Done->Run();
        }

        // 清空表
        _Pending_requests.clear();

        // 等待自动重连
        _Logger.debug("retrying...");
    }
}

void RaftRpcChannel::_OnMessage(const muduo::net::TcpConnectionPtr & _Conn, muduo::net::Buffer * _Buffer, muduo::Timestamp _Receive_time)
{
    while (_Buffer->readableBytes() >= sizeof(RaftRpcFixedHeader)) {
        RaftRpcFixedHeader header;
        // 取出固定头
        ::memcpy(&header, _Buffer->peek(), sizeof(header));

        // 字节序转换
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
            _Logger.error("response deserialization failed");
            return;
        }

        // 根据序列号找到该请求上下文
        std::unique_lock<std::mutex> lock(_Mutex);
        auto it = _Pending_requests.find(sequence_id);
        if (it == _Pending_requests.end()) {
            return;
        }

        CallMethodContext & context = it->second;

        // 反序列化负载部分
        if (!context._Response->ParseFromString(payload)) {
            _Logger.error("parse payload failed");
            context._Controller->SetFailed("parse payload failed");
            context._Done->Run();
            return;
        }

        // 解析成功，清理等待
        _Event_loop->cancel(context._Timer_id);
        _Pending_requests.erase(it);

        lock.unlock();

        // 调用回调函数通知业务层
        context._Done->Run();
    }
}

void RaftRpcChannel::_HandleTimeout(SequenceType _Sequence_id)
{
    // 查找该请求
    std::unique_lock<std::mutex> lock(_Mutex);
    auto it = _Pending_requests.find(_Sequence_id);
    if (it == _Pending_requests.end()) {
        return;
    }

    // 标记失败
    _Logger.error("request timeout");
    CallMethodContext & context = it->second;
    context._Controller->SetFailed("request timeout");

    // 清除请求
    _Pending_requests.erase(it);

    lock.unlock();
    
    context._Done->Run();
}

} // namespace WW
