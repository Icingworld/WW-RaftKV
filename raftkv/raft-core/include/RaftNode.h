#pragma once

#include <RaftLog.h>

namespace WW
{

/**
 * @brief Raft 节点主体
*/
class RaftNode
{
public:
    /**
     * @brief 节点身份
    */
    enum class NodeRole
    {
        Follower,           // 追随者
        Candidate,          // 候选者
        Leader              // 领导者
    };

private:
    // 基本信息
    NodeId _Id;                     // 节点 ID
    TermId _Term;                   // 当前任期
    NodeRole _Role;                 // 当前身份

    // 选举信息
    NodeId _Voted_for;              // 投票目标节点
    NodeId _Leader_id;              // Leader ID

    // 日志信息
    RaftLog _Logs;                  // 日志记录
    LogIndex _Last_commit_index;    // 最新提交日志的索引
    LogIndex _Last_applied_index;   // 最新应用日志的索引

public:
    explicit RaftNode(NodeId _Id);

public:
    /**
     * @brief 获取节点 ID
    */
    NodeId getId() const;

    /**
     * @brief 获取当前任期号
    */
    TermId getTerm() const;

    /**
     * @brief 获取节点身份
    */
    NodeRole getRole() const;

    /**
     * @brief 获取投票节点 ID
    */
    NodeId getVotedFor() const;

    /**
     * @brief 获取 Leader ID
    */
    NodeId getLeaderId() const;

    /**
     * @brief 获取最新提交日志的索引
    */
    LogIndex getLastCommitIndex() const;

    /**
     * @brief 获取最新应用日志的索引
    */
    LogIndex getLastAppliedIndex() const;

    /**
     * @brief 获取最新日志索引
    */
    LogIndex getLastIndex() const;

    /**
     * @brief 获取逻辑索引
    */
    LogIndex getBaseIndex() const;

    /**
     * @brief 获取最新日志任期
    */
    TermId getLastTerm() const;

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
    */
    const RaftLogEntry & getLog(LogIndex _Index) const;

    /**
     * @brief 获取指定索引的任期号
    */
    TermId getTerm(LogIndex _Index) const;

    /**
     * @brief 是否为 Follower
    */
    bool isFollower() const;

    /**
     * @brief 是否为 Candidate
    */
    bool isCandidate() const;

    /**
     * @brief 是否为 Leader
    */
    bool isLeader() const;

    /**
     * @brief 判断日志索引和任期是否匹配
    */
    bool match(LogIndex _Index, TermId _Term) const;

    /**
     * @brief 添加日志
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
     * @brief 获取指定索引之后的日志条目
    */
    std::vector<RaftLogEntry> getLogFrom(LogIndex _Index);

    /**
     * @brief 设置任期号
    */
    void setTerm(TermId term);

    /**
     * @brief 设置节点身份
    */
    void setRole(NodeRole role);

    /**
     * @brief 转换为 Follower
    */
    void switchToFollower();

    /**
     * @brief 转换为 Candidate
    */
    void switchToCandidate();

    /**
     * @brief 转换为 Leader
    */
    void switchToLeader();

    /**
     * @brief 设置投票目标节点
    */
    void setVotedFor(NodeId _Id);

    /**
     * @brief 设置 Leader ID
    */
    void setLeaderId(NodeId _Id);

    /**
     * @brief 设置最新提交日志的索引
    */
    void setLastCommitIndex(LogIndex _Last_commit_index);

    /**
     * @brief 设置最新应用日志的索引
    */
    void setLastAppliedIndex(LogIndex _Last_applied_index);

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
