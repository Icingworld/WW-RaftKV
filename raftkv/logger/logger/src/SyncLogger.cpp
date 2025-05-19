#include "SyncLogger.h"

#include <LogMessage.h>

namespace WW
{

void SyncLogger::log(const std::string & name, LogLevel level, const std::string & message, const char * file, unsigned int line, const char * function)
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

    std::lock_guard<std::mutex> lock(mutex);
    for (auto & sink : sinks) {
        sink->log(msg);
    }
}

void SyncLogger::flush()
{
    std::lock_guard<std::mutex> lock(mutex);
    for (auto & sink : sinks) {
        sink->flush();
    }
}

LogType SyncLogger::getType() const
{
    return LogType::Sync;
}

} // namespace WW
