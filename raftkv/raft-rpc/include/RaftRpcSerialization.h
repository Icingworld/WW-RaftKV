#pragma once

#include <string>

#include <RaftRpcCommon.h>
#include <RaftRpcData.pb.h>

#include <google/protobuf/message.h>

namespace WW
{

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
                          SequenceType _Sequence_id,
                          const google::protobuf::Message & _Args,
                          std::string & _Out_buffer);

    /**
     * @brief 反序列化信息
     * @param _In_buffer 输入序列化字符串
     * @param _Service_name 服务名
     * @param _Method_name 方法名
     * @param _Sequence_id 序列号
     * @param _Payload 输出负载
     */
    static bool deserialize(const std::string & _In_buffer,
                            std::string & _Service_name,
                            std::string & _Method_name,
                            uint64_t & _Sequence_id,
                            std::string & _Payload);
};

} // namespace WW
