#include "TimedFileSink.h"

#include <sstream>
#include <iomanip>

namespace WW
{

TimedFileSink::TimedFileSink(const std::string & filename, const std::string & pattern, const std::chrono::duration<int> & duration, const std::string & file_format)
    : FileSink(filename, pattern)
    , last_time()
    , duration(duration)
    , file_format(file_format)
{
}

TimedFileSink::TimedFileSink(const std::string & filename, const std::chrono::duration<int> & duration, const std::string & file_format, std::shared_ptr<FormatterBase> formatter)
    : FileSink(filename, formatter)
    , last_time()
    , duration(duration)
    , file_format(file_format)
{
    openFile();
}

void TimedFileSink::setDuration(std::chrono::duration<int> duration)
{
    this->duration = duration;
}

void TimedFileSink::setDurationHours(unsigned int hours)
{
    this->duration = std::chrono::hours(hours);
}

void TimedFileSink::setDurationDays(unsigned int days)
{
    this->duration = std::chrono::hours(days * 24);
}

void TimedFileSink::setDurationWeeks(unsigned int weeks)
{
    this->duration = std::chrono::hours(weeks * 24 * 7);
}

void TimedFileSink::log(const LogMessage & msg)
{
    checkRotate();
    FileSink::log(msg);
}

void TimedFileSink::log(const char * data, std::size_t size)
{
    checkRotate();
    FileSink::log(data, size);
}

void TimedFileSink::flush()
{
    FileSink::flush();
}

void TimedFileSink::checkRotate()
{
    auto cur_time = std::chrono::system_clock::now();
    if (shouldRotate(cur_time)) {
        rotate();
    }
}

bool TimedFileSink::shouldRotate(const std::chrono::system_clock::time_point & cur_time)
{
    if (cur_time - last_time > duration) {
        last_time = cur_time;
        return true;
    }
    
    return false;
}

void TimedFileSink::rotate()
{
    if (file.is_open()) {
        file.close();
    }

    openFile();
}

std::string TimedFileSink::formatTime() const
{
    std::time_t time = std::chrono::system_clock::to_time_t(last_time);
    std::tm tm = *std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(&tm, file_format.c_str());

    return oss.str();
}

void TimedFileSink::openFile()
{
    std::string new_file = name + "_" + formatTime() + suffix;
    file.open(new_file, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + new_file);
    }
}

} // namespace WW
