#pragma once

#include <RaftRpcCommon.h>

namespace WW
{

constexpr uint64_t kMagicNumber = 0x0A1B2C3D4E5F6A7B;

/**
 * @brief 固定头部
 * @details 共 16 字节
*/
#pragma pack(push, 1)
class RaftRpcFixedHeader
{
public:
    uint64_t _Magic_number;      // 魔数
    uint32_t _Total_length;      // 报文长度
    uint32_t _Header_checksum;   // 头部校验和

public:
    /**
     * @brief 序列化固定头
     * @details 为了计算头部校验和
    */
    static CRC32Type calculateHeaderChecksum(const RaftRpcFixedHeader & _Header);
};
#pragma pack(pop)

} // namespace WW
