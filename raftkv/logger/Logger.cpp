#include "Logger.h"

namespace WW
{

std::mutex Logger::loggers_mutex;

std::unordered_map<std::string, std::shared_ptr<Logger>> Logger::loggers;

Logger::Logger(const std::string & name)
    : Logger(name, LogType::Sync)
{
}

Logger::Logger(const std::string & name, LogType type)
    : level(LogLevel::Info)
    , name(name)
    , formattered(false)
{
    if (type == LogType::Sync) {
        logger = std::make_shared<SyncLogger>();
    } else if (type == LogType::Async) {
        logger = std::make_shared<AsyncLogger>();
    }
}

Logger::~Logger()
{
    flush();
}

Logger & Logger::getDefaultLogger(const std::string & name)
{
    return getLogger(name, LogType::Sync);
}

Logger & Logger::getSyncLogger(const std::string & name)
{
    return getLogger(name, LogType::Sync);
}

Logger & Logger::getAsyncLogger(const std::string & name)
{
    return getLogger(name, LogType::Async);
}

void Logger::setType(LogType type)
{
    if ((type == LogType::Sync && logger->getType() == LogType::Sync) ||
        (type == LogType::Async && logger->getType() == LogType::Async)) {
        return;
    }

    if (type == LogType::Sync) {
        logger = std::make_shared<SyncLogger>();
    } else if (type == LogType::Async) {
        logger = std::make_shared<AsyncLogger>();
    }
}

void Logger::setLevel(LogLevel level)
{
    this->level = level;
}

void Logger::addSink(std::shared_ptr<SinkBase> sink)
{
    if (formattered) {
        sink->setFormatter(formatter);
    }
    logger->addSink(sink);
}

void Logger::setFormatter(const std::string & pattern)
{
    formatter = std::make_shared<DefaultFormatter>(pattern);
    logger->setFormatter(pattern);
    formattered = true;
}

void Logger::setFormatter(std::shared_ptr<FormatterBase> formatter)
{
    this->formatter = formatter;
    logger->setFormatter(formatter);
    formattered = true;
}

void Logger::clearFormatter()
{
    formattered = false;
}

void Logger::log(LogLevel level, const std::string & message, const char * file, unsigned int line, const char * function)
{
    if (level == LogLevel::Off || level < this->level)
        return;
    
    logger->log(name, level, message, file, line, function);
}

void Logger::trace(const std::string & message, const char * file, unsigned int line, const char * function)
{
    log(LogLevel::Trace, message, file, line, function);
}

void Logger::debug(const std::string & message, const char * file, unsigned int line, const char * function)
{
    log(LogLevel::Debug, message, file, line, function);
}

void Logger::info(const std::string & message, const char * file, unsigned int line, const char * function)
{
    log(LogLevel::Info, message, file, line, function);
}

void Logger::warn(const std::string & message, const char * file, unsigned int line, const char * function)
{
    log(LogLevel::Warn, message, file, line, function);
}

void Logger::error(const std::string & message, const char * file, unsigned int line, const char * function)
{
    log(LogLevel::Error, message, file, line, function);
}

void Logger::fatal(const std::string & message, const char * file, unsigned int line, const char * function)
{
    log(LogLevel::Fatal, message, file, line, function);
}

void Logger::flush()
{
    logger->flush();
}

Logger & Logger::getLogger(const std::string & name, LogType type)
{
    std::lock_guard<std::mutex> lock(loggers_mutex);
    auto it = loggers.find(name);
    if (it == loggers.end()) {
        auto logger = std::shared_ptr<Logger>(new Logger(name, type));
        loggers[name] = logger;
        return *logger;
    }

    return *(it->second);
}

} // namespace WW
