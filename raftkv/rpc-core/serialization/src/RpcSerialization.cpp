
#include "RpcSerialization.h"

#include <arpa/inet.h>

#include <RpcHeader.pb.h>

namespace WW
{

bool RpcSerialization::serialize(const std::string & service_name,
                                  const std::string & method_name,
                                  const google::protobuf::Message & args,
                                  std::string & out_buffer)
{
    // 将 args 序列化为字符串
    std::string args_str;
    if (!args.SerializeToString(&args_str)) {
        return false;
    }

    // 构建 RpcHeader
    RpcHeader header;
    header.set_service_name(service_name);
    header.set_method_name(method_name);
    header.set_args_size(args_str.size());

    // 将 RpcHeader 序列化为字符串
    std::string header_str;
    if (!header.SerializeToString(&header_str)) {
        return false;
    }

    uint32_t header_size = header_str.size();
    // 从主机序转换为网络序
    uint32_t net_header_size = htonl(header_size);

    // 拼接报文，报文格式：[4字节报文头长度][报文头][参数]

    out_buffer.clear();
    out_buffer.append(reinterpret_cast<char*>(&net_header_size), sizeof(uint32_t));
    out_buffer.append(header_str);
    out_buffer.append(args_str);

    return true;
}

bool RpcSerialization::deserialize(const std::string & in_buffer,
                                    std::string & service_name,
                                    std::string & method_name,
                                    std::string & out_buffer)
{
    if (in_buffer.size() < sizeof(uint32_t)) {
        // 长度不够报文头，非法报文
        return false;
    }

    // 提取 header_size
    uint32_t net_header_size = 0;
    memcpy(&net_header_size, in_buffer.data(), sizeof(uint32_t));
    // 从网络序转换为主机序
    uint32_t header_size = ntohl(net_header_size);

    if (in_buffer.size() < sizeof(uint32_t) + header_size) {
        // 后续报文长度不够，非法报文
        return false;
    }

    // 提取 RpcHeader
    RpcHeader header;
    if (!header.ParseFromArray(in_buffer.data() + sizeof(uint32_t), header_size)) {
        return false;
    }

    service_name = header.service_name();
    method_name = header.method_name();
    uint32_t args_size = header.args_size();

    // 检查剩余数据是否足够
    size_t args_offset = sizeof(uint32_t) + header_size;
    if (in_buffer.size() < args_offset + args_size) {
        return false;
    }

    // 将 args 赋值给目标字符串
    out_buffer.assign(in_buffer.data() + args_offset, args_size);

    return true;
}

bool RpcSerialization::deserializeArgs(const std::string & in_buffer, google::protobuf::Message * args)
{
    if (!args->ParseFromString(in_buffer)) {
        return false;
    }

    return true;
}

} // namespace WW
