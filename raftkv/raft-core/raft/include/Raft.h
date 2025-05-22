#pragma once

#include <random>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>

#include <RaftNode.h>
#include <RaftPeer.h>
#include <RaftServiceImpl.h>
#include <RaftServer.h>
#include <raft.pb.h>

namespace WW
{

/**
 * @brief Raft 共识算法
*/
class Raft
{
private:
    // 基本信息
    RaftNode _Node;                     // 本地节点
    std::vector<RaftPeer> _Peers;       // 全部节点信息

    // 服务注册
    RaftServiceImpl _Service;           // 服务实例
    RaftServer _Server;                 // 服务端

    // 选举
    const int _Election_timeout_min;    // 超时下限
    const int _Election_timeout_max;    // 超时上限
    int _Election_timeout;              // 心跳超时间隔
    Timestamp _Last_heartbeat_recv;     // 上一次接收心跳时间

    // 随机数
    std::mt19937 _Rng;
    std::uniform_int_distribution<int> _Election_dist;

    // 心跳
    int _Heartbeat_timeout;             // 心跳发送间隔
    Timestamp _Last_heartbeat_send;     // 上一次发送心跳时间

    // 线程
    std::atomic<bool> _Running;         // 线程是否运行
    std::thread _Client_thread;         // 客户端线程
    std::mutex _Mutex;                  // 用于保护节点的互斥量

public:
    friend class RaftServiceImpl;

    Raft(NodeId _Id, const std::vector<RaftPeer> & _Peers);

public:
    /**
     * @brief 启动 Raft
    */
    void run();

    /**
     * @brief 停止 Raft
    */
    void stop();

private:
    /**
     * @brief 工作线程函数
    */
    void _ClientWorkingThread();

    /**
     * @brief tick
    */
    void _Tick();

    /**
     * @brief 发起选举
    */
    void _StartElection();

    /**
     * @brief 发送心跳
    */
    void _SendHeartbeat();

    /**
     * @brief 处理投票响应的回调函数
    */
    void _OnVoteResponse(const RequestVoteResponse & _Response);

    /**
     * @brief 处理日志响应的回调函数
    */
    void _OnAppendEntriesResponse(const AppendEntriesResponse & _Response);

    /**
     * @brief 处理到来的投票请求
    */
    void _OnVoteRequest(const RequestVoteRequest & _Request, RequestVoteResponse & _Response);

    /**
     * @brief 处理到来的日志请求
    */
    void _OnAppendEntriesRequest(const AppendEntriesRequest & _Request, AppendEntriesResponse & _Response);

    /**
     * @brief 判断候选人的日志是否比自己新
    */
    bool _IsCandidateLogUpToDate(LogIndex _Last_index, TermId _Last_term);
};

} // namespace WW
