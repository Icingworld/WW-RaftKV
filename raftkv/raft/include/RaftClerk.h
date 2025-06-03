#pragma once

#include <thread>
#include <mutex>
#include <atomic>

#include <KVStore.h>
#include <Raft.h>
#include <RaftPeerNet.h>
#include <RaftRpcServiceImpl.h>
#include <RaftRpcServer.h>
#include <RaftRpcClient.h>
#include <Raft.pb.h>
#include <muduo/net/EventLoop.h>

namespace WW
{

/**
 * @brief Raft 应用层
*/
class RaftClerk
{
private:
    friend class RaftRpcServiceImpl;
    friend class RaftOperationServiceImpl;

private:
    Raft * _Raft;                       // Raft 协议
    std::vector<RaftPeerNet> _Peers;    // 存储网络信息
    std::vector<RaftRpcClient *> _Clients;

    KVStore<std::string, std::string> _KVStore;  // KV 储存

    // 定时器线程
    std::atomic<bool> _Running;         // 运行状态
    double _Timeout;                       // 定时器

    // 多线程
    std::mutex _Mutex;                  // 保护 Raft 状态

    // 服务端
    muduo::net::EventLoop _Loop;

    RaftRpcServiceImpl _Service;        // 服务实例
    RaftRpcServer * _Server;            // 服务端
    RaftOperationServiceImpl _Op_service;
    RaftOperationServer * _Op_server;

public:
    RaftClerk(NodeId _Id, const std::vector<RaftPeerNet> & _Peers);

    ~RaftClerk();

public:
    /**
     * @brief 启动 Raft
    */
    void run();

    /**
     * @brief 关闭 Raft
    */
    void stop();

private:
    /**
     * @brief 客户端定时器线程
    */
    void _ClientWorking();

    /**
     * @brief 处理 Raft 中输出的消息
    */
    void _HandleRaftMessageOut(const RaftMessage & _Message);

    /**
     * @brief 处理发送投票请求的消息
    */
    void _SendRequestVoteRequest(const RaftMessage & _Message);

    /**
     * @brief 处理发送日志同步请求的消息
    */
    void _SendAppendEntriesRequest(const RaftMessage & _Message);

    /**
     * @brief 处理应用日志的消息
    */
    void _ApplyLogEntries(const RaftMessage & _Message);

    /**
     * @brief 生成并保存快照
    */
    void _GenerateSnapShot(const RaftMessage & _Message);

    /**
     * @brief 安装快照
    */
    void _InstallSnapShot(const RaftMessage & _Message);

    /**
     * @brief 处理 Rpc 收到的投票请求
    */
    void _HandleRequestVoteRequest(const RequestVoteRequest & _Request, RequestVoteResponse & _Response);

    /**
     * @brief 处理 Rpc 收到的投票响应
    */
    void _HandleRequestVoteResponse(const RequestVoteResponse & _Response);

    /**
     * @brief 处理 Rpc 收到的心跳/日志同步请求
    */
    void _HandleAppendEntriesRequest(const AppendEntriesRequest & _Request, AppendEntriesResponse & _Response);

    /**
     * @brief 处理 Rpc 收到的日志同步响应
    */
    void _HandleAppendEntriesResponse(NodeId _Id, const AppendEntriesResponse & _Response);

    /**
     * @brief 解析并执行命令
    */
    void _ParseAndExecCommand(const std::string & _Command);

    /**
     * @brief 处理 Rpc 收到的操作命令
    */
    void _HandleOperateRaftRequest(const RaftOperationRequest & _Request, RaftOperationResponse & _Response);
};

} // namespace WW
