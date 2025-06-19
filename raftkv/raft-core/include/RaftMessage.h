#pragma once

#include <vector>

#include <RaftLogEntry.h>
#include <RaftCommon.h>

namespace WW
{

/**
 * @brief 用于应用层和 Raft 算法层通信
*/
class RaftMessage
{
public:
    /**
     * @brief 消息类型
    */
    enum class MessageType
    {
        AppendEntriesRequest,               // Leader 向其他节点同步日志请求
        AppendEntriesResponse,              // 收到同步日志响应
        RequestVoteRequest,                 // Raft 发起选举，需要发送投票请求
        RequestVoteResponse,                // 收到投票响应
        InstallSnapshotRequest,             // Leader 向 Follower 发送安装快照请求
        InstallSnapshotResponse,            // 收到安装快照响应
        KVOperationRequest,                 // 收到 KV 操作请求
        KVOPerationResponse,                // KV 操作响应
        ApplyCommitLogsRequest,             // 应用日志到状态机请求
        ApplyCommitLogsResponse,            // 应用日志到状态机响应
        GenerateSnapshotRequest,            // 生成快照请求
        GenerateSnapshotResponse,           // 生成快照响应
        ApplySnapshotRequest,               // 应用快照请求
        ApplySnapshotResponse               // 应用快照响应
    };

    /**
     * @brief KV 操作类型
     */
    enum class OperationType
    {
        PUT,
        UPDATE,
        DELETE,
        GET
    };

public:
    MessageType type;                       // 消息类型

protected:
    explicit RaftMessage(MessageType type);
};

class RaftRequestVoteRequestMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    NodeId from;                                // 消息来自哪个节点
    NodeId to;                                  // 消息送往哪个节点
    TermId term;                                // 任期
    LogIndex last_log_index;                    // 最新日志索引
    TermId last_log_term;                       // 最新日志任期

public:
    RaftRequestVoteRequestMessage();
};

class RaftRequestVoteResponseMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    TermId term;                                // 任期
    bool vote_granted;                          // 是否投票

public:
    explicit RaftRequestVoteResponseMessage();
};

class RaftAppendEntriesRequestMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    TermId term;                                // 任期
    NodeId from;                                // 消息来自哪个节点
    NodeId to;                                  // 消息送往哪个节点
    LogIndex prev_log_index;                    // 前一条日志索引
    TermId prev_log_term;                       // 前一条日志任期
    LogIndex leader_commit;                     // Leader 提交进度
    std::vector<RaftLogEntry> entries;          // 日志条目数组

public:
    RaftAppendEntriesRequestMessage();
};

class RaftAppendEntriesResponseMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    NodeId from;                                // 消息来自哪个节点
    TermId term;                                // 任期
    bool success;                               // 心跳/日志同步是否成功
    LogIndex last_log_index;                    // 同步进度（待优化）

public:
    RaftAppendEntriesResponseMessage();
};

class RaftInstallSnapshotRequestMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    TermId term;                                // 任期
    NodeId from;                                // 消息来自哪个节点
    NodeId to;                                  // 消息送往哪个节点
    LogIndex last_included_index;               // 最后一条被快照压缩的日志索引
    TermId last_included_term;                  // 最后一条被快照压缩的日志任期
    std::string snapshot;                       // 快照内容

public:
    RaftInstallSnapshotRequestMessage();
};

class RaftInstallSnapshotResponseMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    TermId term;                                // 任期
    NodeId from;                                // 消息来自哪个节点

public:
    RaftInstallSnapshotResponseMessage();
};

class KVOperationRequestMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    std::string uuid;                           // 客户端 UUID
    OperationType op_type;                      // 操作类型
    std::string key;                            // 键
    std::string value;                          // 值

public:
    KVOperationRequestMessage();
};

class KVOperationResponseMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    std::string uuid;                           // 客户端 UUID
    NodeId leader_id;                           // Leader ID
    OperationType op_type;                      // 操作类型
    std::string key;                            // 键
    std::string value;                          // 值
    bool success;                               // 是否允许操作

public:
    KVOperationResponseMessage();
};

class ApplyCommitLogsRequestMessage : public RaftMessage
{
public:
    LogIndex last_commit_index;                 // 本次提交的范围
    std::vector<RaftLogEntry> entries;          // 本次提交的日志条目数组

public:
    ApplyCommitLogsRequestMessage();
};

class ApplyCommitLogsResponseMessage : public RaftMessage
{
public:
    LogIndex last_commit_index;                 // 本次提交的范围

public:
    ApplyCommitLogsResponseMessage();
};

class GenerateSnapshotRequestMessage : public RaftMessage
{
public:
    LogIndex last_applied_index;                // 最新应用的日志索引
    TermId last_applied_term;                   // 最新应用的日志任期

public:
    GenerateSnapshotRequestMessage();
};

class GenerateSnapshotResponseMessage : public RaftMessage
{
public:
    LogIndex last_applied_index;                // 最新应用的日志索引
    TermId last_applied_term;                   // 最新应用的日志任期

public:
    GenerateSnapshotResponseMessage();
};

class ApplySnapshotRequestMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    LogIndex last_included_index;               // 最后一条被快照压缩的日志索引
    TermId last_included_term;                  // 最后一条被快照压缩的日志任期
    std::string snapshot;                       // 快照内容

public:
    ApplySnapshotRequestMessage();
};

class ApplySnapshotResponseMessage : public RaftMessage
{
public:
    SequenceType seq;                           // 收到请求的序列号
    LogIndex last_included_index;               // 最后一条被快照压缩的日志索引
    TermId last_included_term;                  // 最后一条被快照压缩的日志任期

public:
    ApplySnapshotResponseMessage();
};

} // namespace WW
