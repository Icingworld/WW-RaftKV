#pragma once

#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

#include <RaftPeer.h>
#include <RaftChannel.h>
#include <RaftLogEntry.h>
#include <Logger.h>

namespace WW
{

/**
 * @brief Raft 共识算法
*/
class Raft
{
public:
    /**
     * @brief 节点身份
    */
    enum class RaftRole
    {
        Follower,           // 追随者
        Candidate,          // 候选者
        Leader              // 领导者
    };

private:
    // 节点信息
    NodeId _Id;             // 节点 ID
    RaftRole _Role;         // 节点身份
    TermId _Term;           // 节点任期

    // 选举信息
    int _Vote_count;        // 选举票数
    NodeId _Voted_for;      // 投票目标节点
    NodeId _Leader_id;      // 记录的 Leader ID

    // 日志信息
    std::vector<RaftLogEntry> _Logs;            // 日志数组
    LogIndex _Base_index;                       // 日志的逻辑索引
    LogIndex _Last_log_index;                   // 最新日志的索引
    TermId _Last_log_term;                      // 最新日志的任期
    LogIndex _Last_included_index;              // 快照包含的最后一条日志的索引
    TermId _Last_included_term;                 // 快照包含的最后一条日志的任期
    LogIndex _Last_commit_index;                // 最新提交日志的索引
    LogIndex _Last_applied_index;               // 最新应用日志的索引

    // 状态控制
    bool _Is_applying;                          // 是否正在应用日志
    bool _Is_snapshoting;                       // 是否正在创建快照

    // 其他节点
    std::vector<RaftPeer> _Peers;

    // 消息队列
    RaftChannel _Inner_channel;                 // 内部消息队列
    RaftChannel _Outter_channel;                // 外部消息队列

    // 定时器
    int _Election_timeout_min;                  // 选举定时器最小间隔
    int _Election_timeout_max;                  // 选举定时器最大间隔
    int _Heartbeat_timeout;                     // 心跳超时间隔
    Timestamp _Election_deadline;               // 选举超时时间
    Timestamp _Heartbeat_deadline;              // 心跳超时时间

    // 线程
    std::atomic<bool> _Running;                 // 是否运行
    std::thread _Raft_thread;                   // 用于驱动 Raft 时间的线程
    std::thread _Message_thread;
    std::mutex _Mutex;

    // 日志
    Logger & _Logger;

public:
    Raft(NodeId _Id, const std::vector<RaftPeer> _Peers);

    ~Raft();

public:
    /**
     * @brief 启动 Raft
    */
    void start();

    void startMessage();

    /**
     * @brief 关闭 Raft
    */
    void stop();

    /**
     * @brief 传入消息
    */
    template <typename RaftMessageType>
    void step(RaftMessageType && _Message)
    {
        _Outter_channel.push(std::forward<RaftMessageType>(_Message));
    }

    /**
     * @brief 从消息队列中读取消息
    */
    std::unique_ptr<RaftMessage> readReady(int _Wait_ms);

    /**
     * @brief 获取本节点 ID
    */
    NodeId getId() const;

    /**
     * @brief 读取持久化文件
    */
    bool loadPersist();

private:
    /**
     * @brief Raft 定时器线程
    */
    void _RaftLoop();

    /**
     * @brief 消息队列线程
    */
    void _GetOutterMessage();

    /**
     * @brief 处理消息
    */
    void _HandleMessage(std::unique_ptr<RaftMessage> _Message);

    /**
     * @brief 转换为 Follower
     * @param _Leader_id Leader 的节点 ID
     * @param _Leader_term Leader 的当前任期
    */
    void _BecomeFollower(NodeId _Leader_id, TermId _Leader_term);

    /**
     * @brief 转换为 Candidate
    */
    void _BecomeCandidate();

    /**
     * @brief 转换为 Leader
    */
    void _BecomeLeader();

    /**
     * @brief 发送心跳/日志同步
    */
    void _SendAppendEntries(bool _Is_heartbeat);

    /**
     * @brief 处理接收到的投票请求
    */
    void _HandleRequestVoteRequest(const RaftRequestVoteRequestMessage * _Message);

    /**
     * @brief 处理接收到的投票响应
    */
    void _HandleRequestVoteResponse(const RaftRequestVoteResponseMessage * _Message);

    /**
     * @brief 处理接收到的日志同步请求
    */
    void _HandleAppendEntriesRequest(const RaftAppendEntriesRequestMessage * _Message);

    /**
     * @brief 处理接收到的日志同步响应
    */
    void _HandleAppendEntriesResponse(const RaftAppendEntriesResponseMessage * _Message);

    /**
     * @brief 处理接收到的快照安装请求
    */
    void _HandleInstallSnapshotRequest(const RaftInstallSnapshotRequestMessage * _Message);

    /**
     * @brief 处理接收到的快照安装响应
    */
    void _HandleInstallSnapshotResponse(const RaftInstallSnapshotResponseMessage * _Message);

    /**
     * @brief 处理接收到的客户端操作请求
    */
    void _HandleKVOperationRequest(const KVOperationRequestMessage * _Message);

    /**
     * @brief 处理应用层返回的日志提交应用响应
    */
    void _HandleApplyCommitLogs(const ApplyCommitLogsResponseMessage * _Message);

    /**
     * @brief 处理应用层返回的快照生成响应
    */
    void _HandleGenerateSnapshot(const GenrateSnapshotResponseMessage * _Message);

    /**
     * @brief 处理应用层返回的快照安装响应
    */
    void _HandleApplySnapshot(const ApplySnapshotResponseMessage * _Message);

    /**
     * @brief 应用已提交日志
    */
    void _ApplyCommitLogs();

    /**
     * @brief 生成快照
    */
    void _GenerateSnapshot();

    /**
     * @brief 检查是否需要生成快照
    */
    void _CheckIfNeedSnapshot();

    /**
     * @brief 随机生成超时时间
     * @param _Timeout_min 最小时间
     * @param _Timeout_max 最大时间
     * @return 随机超时时间
    */
    int _GetRandomTimeout(int _Timeout_min, int _Timeout_max) const;

    void _ResetElectionDeadline();

    void _ResetHeartbeatDeadline();

    TermId _GetTermAt(LogIndex _Index) const;

    bool _LogUpToDate(LogIndex _Last_index, TermId _Last_term) const;

    bool _LogMatch(LogIndex _Index, TermId _Term) const;

    void _TruncateAfter(LogIndex _Truncate_index);

    void _TruncateBefore(LogIndex _Truncate_index);

    const RaftLogEntry & _GetLogAt(LogIndex _Index) const;

    std::vector<RaftLogEntry> _GetLogFrom(LogIndex _Index) const;

    /**
     * @brief 持久化
    */
    void _Persist();
};

} // namespace WW
