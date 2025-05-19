#pragma once

#include <unordered_map>

#include <SyncLogger.h>
#include <AsyncLogger.h>

namespace WW
{

/**
 * @brief 日志
 */
class Logger
{
private:
    LogLevel level;                             // 日志等级
    std::shared_ptr<LoggerBase> logger;         // 日志实例
    std::string name;                           // 日志名称
    bool formattered;                           // 是否设置日志格式化器
    std::shared_ptr<FormatterBase> formatter;   // 日志格式化器
    static std::mutex loggers_mutex;            // 日志哈希表互斥锁
    static std::unordered_map<std::string, std::shared_ptr<Logger>> loggers;    // 日志哈希表

private:
    explicit Logger(const std::string & name);

    Logger(const std::string & name, LogType type);

    Logger(const Logger &) = delete;

    Logger & operator=(const Logger &) = delete;

    Logger(Logger &&) = delete;

    Logger & operator=(Logger &&) = delete;

public:
    ~Logger();

public:
    /**
     * @brief 获取默认日志实例
     */
    static Logger & getDefaultLogger(const std::string & name = "default_logger");

    /**
     * @brief 获取同步日志实例
     */
    static Logger & getSyncLogger(const std::string & name = "sync_logger");

    /**
     * @brief 获取异步日志实例
     */
    static Logger & getAsyncLogger(const std::string & name = "async_logger");

    /**
     * @brief 设置日志类型
     */
    void setType(LogType type);

    /**
     * @brief 设置日志等级
     */
    void setLevel(LogLevel level);

    /**
     * @brief 添加日志输出
     */
    void addSink(std::shared_ptr<SinkBase> sink);

    /**
     * @brief 设置日志格式化器
     */
    void setFormatter(const std::string & pattern);

    /**
     * @brief 设置日志格式化器
     */
    void setFormatter(std::shared_ptr<FormatterBase> formatter);

    /**
     * @brief 清除日志格式化器
     */
    void clearFormatter();

    /**
     * @brief 输出日志
     */
    void log(LogLevel level, const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 输出跟踪日志
     */
    void trace(const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 输出调试日志
     */
    void debug(const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 输出信息日志
     */
    void info(const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 输出警告日志
     */
    void warn(const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 输出错误日志
     */
    void error(const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 输出致命日志
     */
    void fatal(const std::string & message, const char * file = "", unsigned int line = 0, const char * function = "");

    /**
     * @brief 刷新日志输出
     */
    void flush();

public:
    /**
     * @brief 获取日志实例
     */
    static Logger & getLogger(const std::string & name = "default_logger", LogType type = LogType::Sync);
};

} // namespace WW
