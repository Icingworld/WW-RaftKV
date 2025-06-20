#pragma once

#include <memory>
#include <map>
#include <unordered_map>

#include <KVStore.h>
#include <Logger.h>
#include <Raft.h>
#include <RaftPeerNet.h>
#include <RaftRpcClient.h>
#include <RaftRpcService.h>
#include <RaftRpcServer.h>
#include <RaftRpcClosure.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>

namespace WW
{

/**
 * @brief Raft 应用层
*/
class RaftClerk
{
private:
    // Raft 节点信息
    std::unique_ptr<Raft> _Raft;            // Raft 算法层
    std::vector<RaftPeerNet> _Peers;        // 存储网络信息

    // KV 存储
    KVStore<std::string, std::string> _KVStore;

    // 客户端
    std::vector<RaftRpcClient *> _Clients;  // Rpc 客户端长连接
    // 服务端
    std::shared_ptr<muduo::net::EventLoop> _Event_loop_client;  // 循环
    std::unique_ptr<muduo::net::EventLoopThreadPool> _Event_loop_thread_pool;   // 客户端专用线程池
    std::unique_ptr<RaftRpcServiceImpl> _Rpc_service;                // Raft 服务
    std::unique_ptr<RaftRpcServer> _Rpc_server;     // Raft 服务端
    std::unique_ptr<KVOperationServiceImpl> _KVOperation_service;    // KVOperation 服务
    std::unique_ptr<KVOperationServer> _KVOperation_server; // KVOperation 服务端

    // 定时器
    std::atomic<bool> _Running;
    std::thread _Message_thread;
    int _Wait_ms;

    // 序列号表
    std::map<uint64_t, RaftRpcServerClosure *> _Pending_requests;
    std::unordered_map<std::string, std::map<uint64_t, RaftRpcServerClosure *>> _Pending_kv_requests;

    // 日志
    Logger & _Logger;

public:
    RaftClerk(NodeId _Id, const std::vector<RaftPeerNet> & _Peers);

    ~RaftClerk();

public:
    /**
     * @brief 启动 Raft
    */
    void start();

    /**
     * @brief 关闭 Raft
    */
    void stop();

private:
    /**
     * @brief 消息队列线程
    */
    void _GetInnerMessage();

    /**
     * @brief 处理 Raft 中传递出来的消息
     * @param _Message 消息
    */
    void _HandleMessage(std::unique_ptr<RaftMessage> _Message);

    /**
     * @brief 发送投票请求
    */
    void _SendRequestVoteRequest(const RaftRequestVoteRequestMessage * _Message);

    /**
     * @brief 发送投票响应
     */
    void _SendRequestVoteResponse(const RaftRequestVoteResponseMessage * _Message);

    /**
     * @brief 发送心跳/日志同步请求
    */
    void _SendAppendEntriesRequest(const RaftAppendEntriesRequestMessage * _Message);

    /**
     * @brief 发送心跳/日志同步请求
    */
    void _SendAppendEntriesResponse(const RaftAppendEntriesResponseMessage * _Message);

    /**
     * @brief 发送快照安装请求
     */
    void _SendInstallSnapshotRequest(const RaftInstallSnapshotRequestMessage * _Message);

    /**
     * @brief 发送快照安装响应
     */
    void _SendInstallSnapshotResponse(const RaftInstallSnapshotResponseMessage * _Message);

    /**
     * @brief 发送客户端操作响应
     */
    void _SendKVOperationResponse(const KVOperationResponseMessage * _Message);

    /**
     * @brief 处理 Raft 中传出的日志提交应用请求
    */
    void _ApplyCommitLogs(const ApplyCommitLogsRequestMessage * _Message);

    /**
     * @brief 处理 Raft 中传出的快照安装请求
    */
    void _ApplySnapshot(const ApplySnapshotRequestMessage * _Message);

    /**
     * @brief 处理 Raft 中传出的快照压缩请求
    */
    void _GenerateSnapshot(const GenerateSnapshotRequestMessage * _Message);

    /**
     * @brief 从持久化文件安装快照
     * @details 启动时调用
    */
    void _InstallSnapshotFromPersist();

    /**
     * @brief 处理接收到的投票请求
    */
    void _HandleRequestVoteRequest(const RequestVoteRequest * _Request, google::protobuf::Closure * _Done);

    /**
     * @brief 处理接收到的投票响应
    */
    void _HandleRequestVoteResponse(const RequestVoteResponse * _Response, const google::protobuf::RpcController * _Controller);

    /**
     * @brief 处理接收到的心跳/日志同步请求
    */
    void _HandleAppendEntriesRequest(const AppendEntriesRequest * _Request, google::protobuf::Closure * _Done);

    /**
     * @brief 处理接收到的心跳/日志同步响应
    */
    void _HandleAppendEntriesResponse(NodeId _Id, const AppendEntriesResponse * _Response, const google::protobuf::RpcController * _Controller);

    /**
     * @brief 处理接收到的快照安装请求
    */
    void _HandleInstallSnapshotRequest(const InstallSnapshotRequest* _Request, google::protobuf::Closure * _Done);

    /**
     * @brief 处理接收到的快照安装响应
    */
    void _HandleInstallSnapshotResponse(NodeId _Id, const InstallSnapshotResponse * _Response, const google::protobuf::RpcController * _Controller);

    /**
     * @brief 处理接收到的客户端操作请求
    */
    void _HandleKVOperationRequest(const KVOperationRequest * _Request, google::protobuf::Closure * _Done);
};

} // namespace WW
