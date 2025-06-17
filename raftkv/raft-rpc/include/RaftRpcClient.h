#pragma once

#include <functional>

#include <RaftRpcClosure.h>
#include <RaftRpcChannel.h>
#include <Raft.pb.h>

namespace WW
{

/**
 * @brief 用于发出 RPC 请求的客户端
*/
class RaftRpcClient
{
public:
    using RequestVoteClosure = RaftRpcClientClosure<RequestVoteRequest, RequestVoteResponse>;
    using AppendEntriesClosure = RaftRpcClientClosure<AppendEntriesRequest, AppendEntriesResponse>;
    using InstallSnapshotClosure = RaftRpcClientClosure<InstallSnapshotRequest, InstallSnapshotResponse>;

    using RequestVoteCallback = typename RequestVoteClosure::ResponseCallback;
    using AppendEntriesCallback = typename AppendEntriesClosure::ResponseCallback;
    using InstallSnapshotCallback = typename InstallSnapshotClosure::ResponseCallback;

private:
    std::shared_ptr<muduo::net::EventLoop> _Event_loop;
    std::string _Ip;
    std::string _Port;
    std::unique_ptr<RaftService_Stub> _Stub;       // 客户端
    std::unique_ptr<RaftRpcChannel> _Channel;      // 通道

public:
    RaftRpcClient(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port);

    ~RaftRpcClient();

public:
    /**
     * @brief 发起投票请求
     * @param _Request 请求消息体
     * @param _Callback 回调函数
    */
    void RequestVote(std::unique_ptr<RequestVoteRequest> _Request, RequestVoteCallback _Callback);

    /**
     * @brief 发起日志同步请求
     * @param _Request 请求消息体
     * @param _Callback 回调函数
    */
    void AppendEntries(std::unique_ptr<AppendEntriesRequest> _Request, AppendEntriesCallback _Callback);

    /**
     * @brief 发起安装快照请求
     * @param _Request 请求消息体
     * @param _Callback 回调函数
    */
    void InstallSnapshot(std::unique_ptr<InstallSnapshotRequest> _Request, InstallSnapshotCallback _Callback);

    /**
     * @brief 连接 Raft 服务端
    */
    void connect();

    /**
     * @brief 断开与 Raft 服务端的连接
    */
    void disconnect();
};

} // namespace WW
