#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace WW
{

/**
 * @brief 日志索引
 * @details 可改为由容器管理
*/
using LogIndex = int;

/**
 * @brief 节点 ID
*/
using NodeId = int;

/**
 * @brief 任期号
*/
using TermId = std::size_t;

/**
 * @brief 时间戳
*/
using Timestamp = std::chrono::steady_clock::time_point;

/**
 * @brief 序列号
*/
using SequenceType = uint64_t;

} // namespace WW
