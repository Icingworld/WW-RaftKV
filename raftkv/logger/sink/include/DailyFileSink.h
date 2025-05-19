#pragma once

#include <TimedFileSink.h>

namespace WW
{

/**
 * @brief 按天切片的日志输出
 */
class DailyFileSink : public TimedFileSink
{
public:
    DailyFileSink(const std::string & filename, const std::string & pattern);

    explicit DailyFileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter = std::make_shared<DefaultFormatter>());

    ~DailyFileSink() override = default;

public:
    void log(const LogMessage & msg) override;

    void log(const char * data, std::size_t size) override;

    void flush() override;
};

} // namespace WW
