#pragma once

#include <string>

#include <Common.h>

namespace WW
{

/**
 * @brief 日志条目
*/
class RaftLogEntry
{
private:
    LogIndex _Index;        // 日志索引
    TermId _Term;           // 日志任期
    std::string _Command;   // 日志命令

public:
    RaftLogEntry(LogIndex _Index, TermId _Term, const std::string & _Command);

public:
    /**
     * @brief 获取索引
    */
    LogIndex getIndex() const;

    /**
     * @brief 获取任期
    */
    TermId getTerm() const;

    /**
     * @brief 获取命令
    */
    const std::string & getCommand() const;
};

} // namespace WW
