#include "RaftRpcFixedHeader.h"

#include <cstring>

#include <RaftRpcCRC32.h>

namespace WW
{

CRC32Type RaftRpcFixedHeader::calculateHeaderChecksum(const RaftRpcFixedHeader & _Header)
{
    // 计算魔数和总长度的校验和
    char buf[sizeof(uint64_t) + sizeof(uint32_t)];
    ::memcpy(buf, &_Header._Magic_number, sizeof(uint64_t));
    ::memcpy(buf + sizeof(uint64_t), &_Header._Total_length, sizeof(uint32_t));

    // 计算 CRC32 校验和
    RaftRpcCRC32 & crc32 = RaftRpcCRC32::getRaftRpcCRC32();
    return crc32.crc32(buf, sizeof(buf));
}

} // namespace WW
