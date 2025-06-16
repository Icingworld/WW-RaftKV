#pragma once

#include <string>

#include <google/protobuf/message.h>
#include "RaftRpcData.pb.h"

namespace WW
{

/**
 * @brief 固定头部
 * @details 共 16 字节
*/
#pragma pack(push, 1)
struct FixedHeader
{
    uint64_t magic_number;      // 魔数
    uint32_t total_length;      // 报文长度
    uint32_t header_checksum;   // 头部校验和
};
#pragma pack(pop)

/**
 * @brief Rpc 序列化
*/
class RaftRpcSerialization
{
public:
    /**
     * @brief 序列化 Rpc 请求
     * @param _Service_name 服务名
     * @param _Method_name 方法名
     * @param _Sequence_id 序列号
     * @param _Args 消息
     * @param _Out_buffer 输出序列化字符串
     */
    static bool serialize(const std::string & _Service_name,
                          const std::string & _Method_name,
                          unsigned long long _Sequence_id,
                          const google::protobuf::Message & _Args,
                          std::string & _Out_buffer);

    /**
     * @brief 反序列化信息
     * @param _In_buffer 输入序列化字符串
     * @param _Service_name 服务名
     * @param _Method_name 方法名
     * @param _Sequence_id 序列号
     * @param _Payload 负载
     */
    static bool deserialize(const std::string & _In_buffer,
                            std::string & _Service_name,
                            std::string & _Method_name,
                            uint64_t & _Sequence_id,
                            std::string & _Payload);

    /**
     * @brief 序列化固定头
     * @details 为了计算头部校验和
    */
    static uint32_t _CalculateHeaderChecksum(const FixedHeader & _Header);
};

} // namespace WW
