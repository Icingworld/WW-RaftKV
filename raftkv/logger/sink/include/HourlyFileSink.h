#pragma once

#include <TimedFileSink.h>

namespace WW
{

/**
 * @brief 按小时切片的日志输出
 */
class HourlyFileSink : public TimedFileSink
{
public:
    HourlyFileSink(const std::string & filename, const std::string & pattern);

    explicit HourlyFileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter = std::make_shared<DefaultFormatter>());

    ~HourlyFileSink() override = default;

public:
    void log(const LogMessage & msg) override;

    void log(const char * data, std::size_t size) override;

    void flush() override;
};

} // namespace WW
