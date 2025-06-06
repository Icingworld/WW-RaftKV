#pragma once

#include <RaftNode.h>
#include <RaftPeer.h>
#include <RaftMessage.h>

namespace WW
{

/**
 * @brief Raft 共识算法
*/
class Raft
{
private:
    // 基本信息
    RaftNode _Node;                             // Raft 节点
    std::vector<RaftPeer> _Peers;               // 同伴列表

    // 定时器
    int _Election_timeout_min;                  // 选举定时器最小间隔
    int _Election_timeout_max;                  // 选举定时器最大间隔
    int _Election_timeout;                      // 选举定时器
    int _Heartbeat_timeout;                     // 心跳定时器
    int _Election_interval;                     // 选举间隔
    int _Heartbeat_interval;                    // 心跳间隔

    // 选举
    int _Vote_count;                            // 选举票数

    // 快照
    bool _Is_snapshoting;                       // 是否正在创建日志

    // 持久化
    bool _Is_dirty;                             // 数据是否发生变化

    // 消息通道
    std::vector<RaftMessage> _Inner_messages;   // Raft 内部驱动产生的输出消息
    RaftMessage _Outter_messages;               // Raft 外部事件驱动产生的输出消息

public:
    Raft(NodeId _Id, const std::vector<RaftPeer> _Peers);

    ~Raft() = default;

public:
    /**
     * @brief 加载 Raft 持久化文件
    */
    bool load();

    /**
     * @brief 时钟推进
     * @param _Delta_time 推进时间
    */
    void tick(int _Delta_time);

    /**
     * @brief 传入消息
     * @details 这是状态机推进的核心接口
    */
    void step(const RaftMessage & _Message);

    /**
     * @brief 读取 Raft 内部消息输出
    */
    const std::vector<RaftMessage> & readInnerMessage() const;

    /**
     * @brief 读取 Raft 外部消息输出
    */
    const RaftMessage & readOutterMessage() const;

    /**
     * @brief 清空 Raft 内部消息输出
    */
    void clearInnerMessage();

    /**
     * @brief 获取 Raft 节点 ID
    */
    NodeId getId() const;

private:
    /**
     * @brief 选举时间判断
    */
    void _TickElection();

    /**
     * @brief 心跳时间判断
    */
    void _TickHeartbeat();

    /**
     * @brief 发起选举
    */
    void _StartElection();

    /**
     * @brief 发送日志同步请求
     */
    void _SendAppendEntries(bool _IsHeartbeat);

    /**
     * @brief 重置选举定时器
    */
    void _ResetElectionTimeout();

    /**
     * @brief 重置心跳定时器
    */
    void _ResetHeartbeatTimeout();

    /**
     * @brief 处理接收到的投票请求
    */
    void _HandleRequestVoteRequest(const RaftMessage & _Message);

    /**
     * @brief 处理接收到的投票响应
    */
    void _HandleRequestVoteResponse(const RaftMessage & _Message);

    /**
     * @brief 处理接收到的日志同步请求
    */
    void _HandleAppendEntriesRequest(const RaftMessage & _Message);

    /**
     * @brief 处理接收到的日志同步响应
    */
    void _HandleAppendEntriesResponse(const RaftMessage & _Message);

    /**
     * @brief 处理接收到的快照安装请求
    */
    void _HandleInstallSnapshotRequest(const RaftMessage & _Message);

    /**
     * @brief 处理接收到的快照安装响应
    */
    void _HandleInstallSnapshotResponse(const RaftMessage & _Message);

    /**
     * @brief 处理接收到的操作请求
    */
    void _HandleOperationRequest(const RaftMessage & _Message);

    /**
     * @brief 应用快照
    */
    void _ApplySnapshot(const RaftMessage & _Message);

    /**
     * @brief 应用已经提交的日志
     * @param _Commit_index 需要应用到的日志索引
    */
    void _ApplyCommitedLogs();

    /**
     * @brief 生成快照
    */
    void _TakeSnapshot();

    /**
     * @brief 检查是否需要生成快照
     * @param _Index
    */
    void _CheckIfNeedSnapshot(LogIndex _Index);

    /**
     * @brief 随机生成超时时间
     * @param _Timeout_min 最小时间
     * @param _Timeout_max 最大时间
     * @return 随机超时时间
    */
    int _GetRandomTimeout(int _Timeout_min, int _Timeout_max) const;

    /**
     * @brief 判断最新日志是否匹配
    */
    bool _LogUpToDate(LogIndex _Last_index, TermId _Last_term);

    /**
     * @brief 持久化 Raft 状态
    */
    void _Persist();
};

} // namespace WW
