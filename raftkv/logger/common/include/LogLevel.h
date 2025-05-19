#pragma once

namespace WW
{

/**
 * @brief 日志等级
 */
enum class LogLevel
{
    Trace,      // 追踪
    Debug,      // 调试
    Info,       // 信息
    Warn,       // 警告
    Error,      // 错误
    Fatal,      // 致命
    Off         // 关闭
};

} // namespace WW
