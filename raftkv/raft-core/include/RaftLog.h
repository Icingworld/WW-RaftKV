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
    LogIndex _Base_index;               // 第一条日志的逻辑索引
    LogIndex _SnapShot_index;           // 快照截断的最后一条日志索引
    TermId _SnapShot_term;              // 快照截断的最后一条日志任期

public:
    RaftLog();

    ~RaftLog() = default;

public:
    /**
     * @brief 获取最新索引
    */
    LogIndex getLastIndex() const;

    /**
     * @brief 获取逻辑索引
    */
    LogIndex getBaseIndex() const;

    /**
     * @brief 获取最新任期
    */
    TermId getLastTerm() const;

    /**
     * @brief 获取指定索引日志的任期
    */
    TermId getTerm(LogIndex _Index) const;

    /**
     * @brief 获取快照索引
    */
    LogIndex getSnapShotIndex() const;

    /**
     * @brief 获取快照任期
    */
    TermId getSnapShotTerm() const;

    /**
     * @brief 获取指定索引的日志
     * @param _Index 索引
    */
    const RaftLogEntry & at(LogIndex _Index) const;

    /**
     * @brief 获取指定索引之后的日志条目
    */
    std::vector<RaftLogEntry> getLogFrom(LogIndex _Index) const;

    /**
     * @brief 判断索引和任期是否匹配
    */
    bool match(LogIndex _Index, TermId _Term) const;

    /**
     * @brief 插入新日志
    */
    void append(const RaftLogEntry & _Log_entry);

    /**
     * @brief 从某处开始截断日志
     * @param _Truncate_index 需要截断的索引
    */
    void truncateAfter(LogIndex _Truncate_index);

    /**
     * @brief 截断某处前面的日志
     * @param _Truncate_index 需要截断的索引
    */
    void truncateBefore(LogIndex _Truncate_index);

    /**
     * @brief 设置快照索引
    */
    void setSnapShotIndex(LogIndex _SnapShot_index);

    /**
     * @brief 设置快照任期
    */
    void setSnapShotTerm(TermId _SnapShot_term);
};

} // namespace WW
