#pragma once

#include <memory>

#include <DefaultFormatter.h>
#include <LogMessage.h>

namespace WW
{

/**
 * @brief 日志输出接口
 */
class SinkBase
{
protected:
    std::shared_ptr<FormatterBase> formatter;   // 日志格式化器

public:
    explicit SinkBase(const std::string & pattern);

    explicit SinkBase(std::shared_ptr<FormatterBase> formatter = std::make_shared<DefaultFormatter>());

    virtual ~SinkBase() = default;

public:
    /**
     * @brief 输出日志
     */
    virtual void log(const LogMessage & msg) = 0;

    /**
     * @brief 刷新日志输出
     */
    virtual void flush() = 0;

    /**
     * @brief 设置日志格式化器
     */
    void setFormatter(std::shared_ptr<FormatterBase> formatter);

    /**
     * @brief 设置日志格式化器
     */
    void setFormatter(const std::string & pattern);
};

} // namespace WW
