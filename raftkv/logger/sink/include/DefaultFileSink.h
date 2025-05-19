#pragma once

#include <fstream>

#include <FileSink.h>

namespace WW
{

/**
 * @brief 默认文件日志输出
 */
class DefaultFileSink : public FileSink
{
public:
    DefaultFileSink(const std::string & filename, const std::string & pattern);

    explicit DefaultFileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter = std::make_shared<DefaultFormatter>());

    ~DefaultFileSink() override;

public:
    /**
     * @brief 输出日志到文件
     */
    void log(const LogMessage & msg) override;

    /**
     * @brief 输出日志到文件
     */
    void log(const char * data, std::size_t size);

    void flush() override;
};

} // namespace WW
