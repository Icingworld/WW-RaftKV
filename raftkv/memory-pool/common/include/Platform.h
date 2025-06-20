#pragma once

#include <MemoryCommon.h>

namespace WW
{

/**
 * @brief 平台接口
*/
class Platform
{
public:
    /**
     * @brief 从堆中以对齐方法获取内存
     * @param _Alignment 对齐大小
     * @param _Size 获取内存大小
    */
    static void * alignedMalloc(size_type _Alignment, size_type _Size);

    /**
     * @brief 释放以对齐方法获取的内存
     * @param _Ptr 释放内存的指针
    */
    static void alignedFree(void * _Ptr);
};

} // namespace WW
