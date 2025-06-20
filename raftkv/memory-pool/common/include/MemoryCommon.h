#pragma once

#include <cstddef>

namespace WW
{

/**
 * @brief 大小类型
 * @details `std::size_t`
 */
using size_type = std::size_t;

/**
 * @brief 页大小
 * @details 大小与系统有关，暂时设置为4K
 */
constexpr size_type PAGE_SIZE = 4096;

/**
 * @brief 页位移
 * @details 用于页号计算，和页大小对应
 */
constexpr size_type PAGE_SHIFT = 12;

/**
 * @brief 最大页数
 * @details 页缓存最多管理128页的页段
 */
constexpr size_type MAX_PAGE_NUM = 128;

/**
 * @brief 内存块数组大小
 * @details 该数值和线程缓存、中心缓存中的内存块大小间隔有关
 */
constexpr size_type MAX_ARRAY_SIZE = 208;

/**
 * @brief 内存池可以管理的最大内存
 */
constexpr size_type MAX_MEMORY_SIZE = PAGE_SIZE * MAX_PAGE_NUM / 2;

/**
 * @brief 线程缓存向中心缓存申请内存块，每次最多512个
 */
constexpr size_type MAX_BLOCK_NUM = 512;

} // namespace WW
