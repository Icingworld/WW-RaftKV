#pragma once

#include <cstddef>

#include <RaftRpcCommon.h>

namespace WW
{

/**
 * @brief CRC32 工具类
*/
class RaftRpcCRC32
{
private:
    CRC32Type _Table[256];       // CRC 表

private:
    RaftRpcCRC32();

    RaftRpcCRC32(const RaftRpcCRC32 & _Other) = delete;

    RaftRpcCRC32 & operator=(const RaftRpcCRC32 & _Other) = delete;

public:
    ~RaftRpcCRC32() = default;

public:
    /**
     * @brief 获取 CRC32 工具单例
    */
    static RaftRpcCRC32 & getRaftRpcCRC32();

    /**
     * @brief 计算 CRC32 值
     * @param _Data 原始数据
     * @param _Length 数据长度
    */
    CRC32Type crc32(const void * _Data, std::size_t _Length) const;

private:
    /**
     * @brief 初始化 CRC 表
    */
    void _Init();
};

} // namespace WW
