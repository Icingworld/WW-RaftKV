#pragma once

#include <string>
#include <cstdint>
#include <memory>

#include <google/protobuf/message.h>

namespace WW
{

/**
 * @brief 序列化器
 */
class RpcSerialization
{
public:
    /**
     * @brief 序列化信息
     * @param service_name 服务名
     * @param method_name 方法名
     * @param args 消息
     * @param out_buffer 输出序列化字符串
     */
    static bool serialize(const std::string & service_name,
                          const std::string & method_name,
                          const google::protobuf::Message & args,
                          std::string & out_buffer);

    /**
     * @brief 反序列化信息
     * @param in_buffer 输入序列化字符串
     * @param service_name 服务名
     * @param method_name 方法名
     * @param out_buffer 剩余参数部分的字符串
     */
    static bool deserialize(const std::string & in_buffer,
                            std::string & service_name,
                            std::string & method_name,
                            std::string & out_buffer);

    /**
     * @brief 反序列化参数
     * @param in_buffer 输入序列化字符串参数部分
     * @param args 输出消息
     */
    static bool deserializeArgs(const std::string & in_buffer, google::protobuf::Message * args);
};

} // namespace WW
