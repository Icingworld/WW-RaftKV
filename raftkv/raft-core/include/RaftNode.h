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
     * @brief 获取最新提交日志的索引
    */
    LogIndex getLastCommitIndex() const;

    /**
     * @brief 获取最新应用日志的索引
    */
    LogIndex getLastAppliedIndex() const;

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
    void setVotedFor(NodeId node);

    /**
     * @brief 设置最新提交日志的索引
    */
    void setLastCommitIndex(LogIndex _Last_commit_index);

    /**
     * @brief 设置最新应用日志的索引
    */
    void setLastAppliedIndex(LogIndex _Last_applied_index);
};

} // namespace WW
