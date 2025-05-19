#include "HourlyFileSink.h"

namespace WW
{

HourlyFileSink::HourlyFileSink(const std::string & filename, const std::string & pattern)
    : TimedFileSink(filename, pattern, std::chrono::hours(1), "%Y-%m-%d_%H")
{
}

HourlyFileSink::HourlyFileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter)
    : TimedFileSink(filename, std::chrono::hours(1), "%Y-%m-%d_%H", formatter)
{
}

void HourlyFileSink::log(const LogMessage & msg)
{
    TimedFileSink::log(msg);
}

void HourlyFileSink::log(const char * data, std::size_t size)
{
    TimedFileSink::log(data, size);
}

void HourlyFileSink::flush()
{
    TimedFileSink::flush();
}

} // namespace WW
