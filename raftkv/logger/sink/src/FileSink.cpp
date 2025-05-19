#include "FileSink.h"

namespace WW
{

FileSink::FileSink(const std::string & filename, const std::string & pattern)
    : SinkBase(pattern)
    , filename(filename)
{
    splitFilename();
}

FileSink::FileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter)
    : SinkBase(std::move(formatter)), 
    filename(filename)
{
    splitFilename();
}

FileSink::~FileSink()
{
    if (file.is_open()) {
        file.close();
    }
}

void FileSink::log(const LogMessage & msg)
{
    if (!file.is_open()) {
        return;
    }

    file << formatter->format(msg) << std::endl;
}

void FileSink::log(const char * data, std::size_t size)
{
    if (file.is_open()) {
        file.write(data, size);
    }
}

void FileSink::flush()
{
    if (file.is_open()) {
        file.flush();
    }
}

void FileSink::splitFilename()
{
    std::size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        name = filename;
        suffix = "";
    } else {
        name = filename.substr(0, dot_pos);
        suffix = filename.substr(dot_pos);
    }
}

void FileSink::openFile()
{
    file.open(filename, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

} // namespace WW
