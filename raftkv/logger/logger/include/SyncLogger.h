#pragma once

#include <mutex>

#include <LoggerBase.h>

namespace WW
{

/**
 * @brief 同步日志
 */
class SyncLogger : public LoggerBase
{
private:
    std::mutex mutex;   // 互斥锁

public:
    SyncLogger() = default;

    ~SyncLogger() override = default;

public:
    void log(const std::string & name, LogLevel level, const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "") override;

    void flush() override;

    LogType getType() const override;
};

} // namespace WW
