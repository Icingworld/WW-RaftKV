#include "DefaultFormatter.h"

#include <iomanip>

namespace WW
{

DefaultFormatter::DefaultFormatter()
    : DefaultFormatter("[%n][%c][%L] %v")
{
}

DefaultFormatter::DefaultFormatter(const std::string & pattern)
    : timed(false)
{
    compile(pattern);
}

std::string DefaultFormatter::format(const LogMessage & msg)
{
    std::ostringstream oss;
    for (const auto & instruction : instructions) {
        instruction(oss, msg);
    }
    timed = false;
    return oss.str();
}

void DefaultFormatter::compile(const std::string & pattern)
{
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == '%' && i + 1 < pattern.size()) {
            char code = pattern[i + 1];
            switch (code) {
                case 'Y':
                case 'y':
                case 'b':
                case 'h':
                case 'B':
                case 'm':
                case 'd':
                case 'e':
                case 'a':
                case 'A':
                case 'w':
                case 'u':
                case 'H':
                case 'I':
                case 'M':
                case 'S':
                case 'c':
                case 'D':
                case 'F':
                case 'T':
                case 'P':
                case 'Z':
                    instructions.emplace_back([this, code](std::ostringstream & oss, const LogMessage & msg) {
                        if (!timed) {
                            getTime(msg, tm_time);
                            timed = true;
                        }
                        char fmt_buf[3] = { '%', code, '\0' };
                        oss << std::put_time(&tm_time, fmt_buf);
                    });
                    break;
                case 'L':
                    instructions.emplace_back([this](std::ostringstream & oss, const LogMessage & msg) {
                        oss << formatLogLevel(msg.level);
                    });
                    break;
                case 't':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.thread_id;
                    });
                    break;
                case 'f':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.file;
                    });
                    break;
                case 'l':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.line;
                    });
                    break;
                case 'C':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.function;
                    });
                    break;
                case 'V':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.file << ":" << msg.line << "-" << msg.function;
                    });
                    break;
                case 'n':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.name;
                    });
                    break;
                case 'v':
                    instructions.emplace_back([](std::ostringstream & oss, const LogMessage & msg) {
                        oss << msg.message;
                    });
                    break;
                default:
                instructions.emplace_back([code](std::ostringstream & oss, const LogMessage & msg) {
                    oss << '%' << code;
                });
            }
            ++i;
        } else {
            size_t start = i;
            while (i < pattern.size() && !(pattern[i] == '%' && i + 1 < pattern.size())) {
                ++i;
            }
            std::string literal = pattern.substr(start, i - start);
            instructions.emplace_back([literal](std::ostringstream& oss, const LogMessage&) {
                oss << literal;
            });
            --i;
        }
    }
}

std::string DefaultFormatter::formatLogLevel(LogLevel level) const
{
    switch (level) {
    case LogLevel::Trace:
        return "trace";
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warn:
        return "warn";
    case LogLevel::Error:
        return "error";
    case LogLevel::Fatal:
        return "fatal";
    default:
        return "unknown";
    }
}

void DefaultFormatter::getTime(const LogMessage & msg, std::tm & tm_time)
{
    time_t time = std::chrono::system_clock::to_time_t(msg.timestamp);
#ifdef _WIN32
    localtime_s(&tm_time, &time);   // Windows
#else
    localtime_r(&time, &tm_time);   // Linux/Mac
#endif
}

} // namespace WW
