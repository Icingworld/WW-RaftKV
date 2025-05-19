#pragma once

#include <FileSink.h>

namespace WW
{

/**
 * @brief 按文件大小轮转的日志文件输出
 */
class RotateFileSink : public FileSink
{
private:
    std::size_t max_size;       // 最大文件大小
    std::size_t max_files;      // 最多文件数量

public:
    RotateFileSink(const std::string & filename, const std::string & pattern, std::size_t max_size = 1024 * 1024, std::size_t max_files = 1);

    explicit RotateFileSink(const std::string & filename, std::size_t max_size = 1024 * 1024, std::size_t max_files = 1, std::shared_ptr<FormatterBase> formatter = std::make_shared<DefaultFormatter>());

    ~RotateFileSink() override = default;

public:
    void log(const LogMessage & msg) override;

    void log(const char * data, std::size_t size) override;

    void flush() override;

private:
    /**
     * @brief 检查是否需要轮转
     */
    void checkRotate();

    /**
     * @brief 获取当前日志文件大小
     */
    std::size_t getCurrentFileSize() const;

    /**
     * @brief 轮转日志文件
     */
    void rotate();
};

} // namespace WW
