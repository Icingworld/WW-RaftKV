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
    RaftLog _Logs;                  // 日志记录

    // 选举信息
    NodeId _Voted_for;              // 投票目标节点
    int _Vote_count;                // 收到的选票数量

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
     * @brief 获取票数
    */
    int getVoteCount() const;

    /**
     * @brief 获取最新日志索引
    */
    LogIndex getLastLogIndex() const;

    /**
     * @brief 获取最新日志任期
    */
    TermId getLastLogTerm() const;

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
     * @brief 设置投票目标节点
    */
    void setVotedFor(NodeId node);

    /**
     * @brief 设置票数
    */
    void setVoteCount(int _Vote_count);
};

} // namespace WW
