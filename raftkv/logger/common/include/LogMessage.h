#pragma once

#include <string>
#include <chrono>
#include <thread>

#include <LogLevel.h>

namespace WW
{

/**
 * @brief 单条日志消息
 */
class LogMessage
{
public:
    std::string name;                                   // 日志名称
    LogLevel level;                                     // 日志等级
    std::string message;                                // 日志消息
    std::string file;                                   // 文件名
    unsigned int line;                                  // 行号
    std::string function;                               // 函数名
    std::chrono::system_clock::time_point timestamp;    // 时间戳
    std::thread::id thread_id;                          // 线程ID
};

} // namespace WW
