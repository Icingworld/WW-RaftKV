#pragma once

#include <fstream>

#include <SinkBase.h>

namespace WW
{

class FileSink : public SinkBase
{
protected:
    std::string filename;                   // 日志文件全名
    std::string name;                       // 日志文件名称
    std::string suffix;                     // 日志文件后缀
    std::ofstream file;                     // 日志文件

public:
    FileSink(const std::string & filename, const std::string & pattern);

    explicit FileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter = std::make_shared<DefaultFormatter>());

    ~FileSink() override;

protected:
    /**
     * @brief 输出日志到文件
     */
    void log(const LogMessage & msg) override;

    /**
     * @brief 输出日志到文件
     */
    virtual void log(const char * data, std::size_t size);

    void flush() override;

private:
    /**
     * @brief 解析日志文件名称
     */
    void splitFilename();

protected:
    /**
     * @brief 打开日志文件
     */
    virtual void openFile();
};

} // namespace WW
