#include "RaftRpcSerialization.h"

#include <RaftRpcCRC32.h>

#include <arpa/inet.h>

namespace WW
{

bool RaftRpcSerialization::serialize(const std::string & _Service_name,
                                     const std::string & _Method_name,
                                     SequenceType _Sequence_id,
                                     const google::protobuf::Message & _Args,
                                     std::string & _Out_buffer)
{
    // 将 args 序列化为字符串
    std::string args_str;
    if (!_Args.SerializeToString(&args_str)) {
        return false;
    }

    // 获取 CRC32 工具实例
    RaftRpcCRC32 & crc32 = RaftRpcCRC32::getRaftRpcCRC32();

    // 计算负载校验和
    CRC32Type payload_checksum = crc32.crc32(args_str.c_str(), args_str.size());

    // 构造 Rpc 结构
    RaftRpcData rpc_data;
    rpc_data.set_service_name(_Service_name);
    rpc_data.set_method_name(_Method_name);
    rpc_data.set_sequence_id(_Sequence_id);
    rpc_data.set_payload_size(args_str.size());
    rpc_data.set_payload_checksum(payload_checksum);
    rpc_data.set_payload(args_str);

    // 序列化
    if (!rpc_data.SerializeToString(&_Out_buffer)) {
        return false;
    }

    return true;
}

bool RaftRpcSerialization::deserialize(const std::string & _In_buffer,
                                       std::string & _Service_name,
                                       std::string & _Method_name,
                                       uint64_t & _Sequence_id,
                                       std::string & _Payload)
{
    // 反序列化
    RaftRpcData rpc_data;
    if (!rpc_data.ParseFromString(_In_buffer)) {
        return false;
    }

    // 验证校验和
    RaftRpcCRC32 & crc32 = RaftRpcCRC32::getRaftRpcCRC32();

    // 验证负载校验和
    _Payload = rpc_data.payload();
    CRC32Type payload_checksum = crc32.crc32(_Payload.c_str(), _Payload.size());

    if (payload_checksum != rpc_data.payload_checksum()) {
        return false;
    }

    // 提取字段
    _Service_name = rpc_data.service_name();
    _Method_name = rpc_data.method_name();
    _Sequence_id = rpc_data.sequence_id();

    return true;
}

} // namespace WW
