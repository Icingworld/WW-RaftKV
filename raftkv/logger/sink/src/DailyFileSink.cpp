#include "DailyFileSink.h"

namespace WW
{

DailyFileSink::DailyFileSink(const std::string & filename, const std::string & pattern)
    : TimedFileSink(filename, pattern, std::chrono::hours(24), "%Y-%m-%d")
{
}

DailyFileSink::DailyFileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter)
    : TimedFileSink(filename, std::chrono::hours(24), "%Y-%m-%d", formatter)
{
}

void DailyFileSink::log(const LogMessage & msg)
{
    TimedFileSink::log(msg);
}

void DailyFileSink::log(const char * data, std::size_t size)
{
    TimedFileSink::log(data, size);
}

void DailyFileSink::flush()
{
    TimedFileSink::flush();
}

} // namespace WW
