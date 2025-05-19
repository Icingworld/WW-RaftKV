#pragma once

#include <Common.h>

namespace WW
{

/**
 * @brief 大小
 * @details 用于提供索引和大小相互转换的规则
 */
class Size
{
public:
    /**
     * @brief 根据数组索引获取内存块大小
     * @param _Index 数组索引
     * @return 内存块大小
     */
    static size_type index_to_size(size_type _Index) noexcept;

    /**
     * @brief 根据内存块大小获取数组索引
     * @param _Size 内存块大小
     * @return 数组索引
     * @details `_Size`是对齐之后的大小
     */
    static size_type size_to_index(size_type _Size) noexcept;

    /**
     * @brief 对齐内存块大小
     * @param _Size 内存块大小
     * @return 对齐后的内存块大小
     */
    static size_type round_up(size_type _Size) noexcept;
};

} // namespace WW
