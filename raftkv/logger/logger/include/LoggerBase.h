#pragma once

#include <string>
#include <vector>

#include <SinkBase.h>
#include <LogLevel.h>

namespace WW
{

/**
 * @brief 日志类型
 */
enum class LogType
{
    Sync,       // 同步
    Async       // 异步
};

/**
 * @brief 日志类接口
 */
class LoggerBase
{
protected:
    std::vector<std::shared_ptr<SinkBase>> sinks;   // 输出接口列表

public:
    LoggerBase() = default;

    virtual ~LoggerBase() = default;

public:
    /**
     * @brief 输出日志
     */
    virtual void log(const std::string & name, LogLevel level, const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "") = 0;

    /**
     * @brief 刷新日志输出
     */
    virtual void flush() = 0;

    /**
     * @brief 获取日志类型
     */
    virtual LogType getType() const = 0;

    /**
     * @brief 添加输出接口
     */
    void addSink(std::shared_ptr<SinkBase> sink);

    /**
     * @brief 设置日志格式化器
     */
    void setFormatter(const std::string & pattern);

    /**
     * @brief 设置日志格式化器
     */
    void setFormatter(std::shared_ptr<FormatterBase> formatter);
};

} // namespace WW
