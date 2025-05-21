#pragma once

#include <chrono>

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
using TermId = int;

/**
 * @brief 时间戳
*/
using Timestamp = std::chrono::steady_clock::time_point;

} // namespace WW
