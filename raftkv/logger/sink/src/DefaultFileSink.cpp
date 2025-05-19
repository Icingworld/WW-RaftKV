#include "DefaultFileSink.h"

namespace WW
{

DefaultFileSink::DefaultFileSink(const std::string & filename, const std::string & pattern)
    : FileSink(filename, pattern)
{
    openFile();
}

DefaultFileSink::DefaultFileSink(const std::string & filename, std::shared_ptr<FormatterBase> formatter)
    : FileSink(filename, formatter)
{
    openFile();
}

DefaultFileSink::~DefaultFileSink()
{
    if (file.is_open()) {
        file.close();
    }
}

void DefaultFileSink::log(const LogMessage & msg)
{
    FileSink::log(msg);
}

void DefaultFileSink::log(const char * data, std::size_t size)
{
    FileSink::log(data, size);
}

void DefaultFileSink::flush()
{
    FileSink::flush();
}

} // namespace WW
