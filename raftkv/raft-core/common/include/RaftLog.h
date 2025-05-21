#pragma once

#include <vector>

#include <RaftLogEntry.h>

namespace WW
{

/**
 * @brief Raft 日志组
*/
class RaftLog
{
private:
    std::vector<RaftLogEntry> _Logs;    // 日志数组
    LogIndex _Last_index;               // 最新日志的索引
    TermId _Last_term;                  // 最新日志的任期

public:
    RaftLog();

    ~RaftLog() = default;

public:
    /**
     * @brief 获取最新索引
    */
    LogIndex getLastIndex() const;

    /**
     * @brief 获取最新任期
    */
    TermId getLastTerm() const;

    /**
     * @brief 获取指定索引的日志
     * @param _Index 索引
    */
    const RaftLogEntry & at(LogIndex _Index) const;

    /**
     * @brief 插入新日志
    */
    void push(const RaftLogEntry & _Log_entry);
};

} // namespace WW
