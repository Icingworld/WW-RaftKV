#include "AsyncLogger.h"

#include <LogMessage.h>
#include <DefaultFileSink.h>

namespace WW
{

AsyncLogger::AsyncLogger()
    : formatter(std::make_shared<DefaultFormatter>())
    , worker(std::bind(&AsyncLogger::callback, this, std::placeholders::_1, std::placeholders::_2))
{
}

AsyncLogger::~AsyncLogger()
{
    worker.stop();
}

void AsyncLogger::log(const std::string & name, LogLevel level, const std::string & message, const char * file, unsigned int line, const char * function)
{
    LogMessage msg = {
        name,
        level,
        message,
        file,
        line,
        function,
        std::chrono::system_clock::now(),
        std::this_thread::get_id()
    };

    std::string data = formatter->format(msg) + "\n";
    worker.push(data.c_str(), data.size());
}

void AsyncLogger::flush()
{
    std::lock_guard<std::mutex> lock(mutex);
    for (auto & sink : sinks) {
        sink->flush();
    }
}

LogType AsyncLogger::getType() const
{
    return LogType::Async;
}

void AsyncLogger::setFormatter(const std::string & pattern)
{
    this->formatter = std::make_shared<DefaultFormatter>(pattern);
}

void AsyncLogger::setFormatter(std::shared_ptr<FormatterBase> formatter)
{
    this->formatter = formatter;
}

void AsyncLogger::callback(const char * data, std::size_t size)
{
    std::lock_guard<std::mutex> lock(mutex);
    for (auto & sink : sinks) {
        auto file_sink = std::dynamic_pointer_cast<WW::DefaultFileSink>(sink);
        if (file_sink) {
            file_sink->log(data, size);
        }
    }
}

} // namespace WW
