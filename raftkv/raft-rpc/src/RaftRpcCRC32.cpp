#include "RaftRpcCRC32.h"

namespace WW
{

RaftRpcCRC32::RaftRpcCRC32()
{
    _Init();
}

RaftRpcCRC32 & RaftRpcCRC32::getRaftRpcCRC32()
{
    static RaftRpcCRC32 crc32;
    return crc32;
}

uint32_t RaftRpcCRC32::crc32(const void * _Data, std::size_t _Length) const
{
    const uint8_t * bytes = reinterpret_cast<const uint8_t *>(_Data);
    uint32_t crc = 0xFFFFFFFF;
    for (std::size_t i = 0; i < _Length; ++i) {
        crc = (crc >> 8) ^ _Table[(crc & 0xFF) ^ bytes[i]];
    }
    return crc ^ 0xFFFFFFFF;
}

void RaftRpcCRC32::_Init()
{
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        _Table[i] = c;
    }
}

} // namespace WW
