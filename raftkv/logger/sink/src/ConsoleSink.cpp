#include "ConsoleSink.h"

#include <iostream>

namespace WW
{

ConsoleSink::ConsoleSink(const std::string & pattern)
    : SinkBase(pattern)
{
}

ConsoleSink::ConsoleSink(std::shared_ptr<FormatterBase> formatter)
    : SinkBase(formatter)
{
}

void ConsoleSink::log(const LogMessage & msg)
{
    if (msg.level == LogLevel::Error || msg.level == LogLevel::Fatal)
        std::cerr << formatter->format(msg) << std::endl;
    else
        std::cout << formatter->format(msg) << std::endl;
}

void ConsoleSink::flush()
{
    std::cout.flush();
    std::cerr.flush();
}

} // namespace WW
