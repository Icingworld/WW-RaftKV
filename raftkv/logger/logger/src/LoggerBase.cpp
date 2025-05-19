
#include "LoggerBase.h"

namespace WW
{

void LoggerBase::addSink(std::shared_ptr<SinkBase> sink)
{
    sinks.emplace_back(std::move(sink));
}

void LoggerBase::setFormatter(const std::string & pattern)
{
    for (auto & sink : sinks) {
        sink->setFormatter(pattern);
    }
}

void LoggerBase::setFormatter(std::shared_ptr<FormatterBase> formatter)
{
    for (auto & sink : sinks) {
        sink->setFormatter(formatter);
    }
}

} // namespace WW
