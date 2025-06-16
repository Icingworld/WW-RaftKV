#pragma once

#include <vector>

#include <RaftLogEntry.h>

namespace WW
{

/**
 * @brief 用于应用层和 Raft 算法层通信
*/
class RaftMessage
{
public:
    enum class MessageType
    {
        AppendEntriesRequest,               // Leader 向其他节点同步日志请求
        AppendEntriesResponse,              // 收到同步日志响应
        RequestVoteRequest,                 // Raft 发起选举，需要发送投票请求
        RequestVoteResponse,                // 收到投票响应
        InstallSnapshotRequest,             // Leader 向 Follower 发送安装快照请求
        InstallSnapshotResponse,            // 收到安装快照响应
        GenerateSnapshot,                   // 生成快照
        ApplySnapshot,                      // 应用快照
        ApplyCommitLogs,                    // 应用日志到状态机
        KVOperationRequest,                 // 收到操作请求
        KVOPerationResponse                 // 操作响应
    };

    enum class OperationType
    {
        PUT,
        UPDATE,
        DELETE,
        GET
    };

public:
    uint64_t seq;                           // 收到请求的序列号
    std::string uuid;                       // 客户端 UUID

    MessageType type;                       // 消息类型
    NodeId from;                            // 消息来自哪个节点
    NodeId to;                              // 消息送往哪个节点
    TermId term;                            // 消息来自哪个任期
    LogIndex index;                         // 日志索引
    TermId log_term;                        // 日志任期
    LogIndex commit;                        // 提交日志索引
    std::vector<RaftLogEntry> entries;      // 日志条目数组
    bool success;                           // 是否成功

    OperationType op_type;                  // 操作类型
    std::string key;                        // 操作键
    std::string value;                      // 操作值
    std::string snapshot;                   // 快照内容
};

} // namespace WW
