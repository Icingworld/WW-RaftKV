#pragma once

#include <vector>
#include <functional>
#include <sstream>

#include <FormatterBase.h>

namespace WW
{

/**
 * @brief 默认格式化器
 */
class DefaultFormatter : public FormatterBase
{
protected:
    std::tm tm_time;            // 用于lambda函数的格式化时间
    bool timed;                 // 是否格式化时间
    std::vector<std::function<void(std::ostringstream &, const LogMessage &)>> instructions;    // 格式化指令集

public:
    DefaultFormatter();

    DefaultFormatter(const std::string & pattern);

    ~DefaultFormatter() override = default;

public:
    std::string format(const LogMessage & msg) override;

private:
    /**
     * @brief 预编译格式化指令集
     */
    void compile(const std::string & pattern);

    /**
     * @brief 格式化日志等级
     */
    std::string formatLogLevel(LogLevel level) const;

    /**
     * @brief 获取日志中的时间并格式化
     */
    void getTime(const LogMessage & msg, std::tm & tm_time);
};

} // namespace WW
