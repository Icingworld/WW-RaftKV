#pragma once

#include <functional>

#include <RaftRpcChannel.h>
#include <Raft.pb.h>

namespace WW
{

/**
 * @brief 用于发出 RPC 请求的客户端
*/
class RaftRpcClient
{
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
     * @param request 请求消息体
     * @param callback 回调函数
    */
    void RequestVote(const RequestVoteRequest * _Request,
                     std::function<void(const RequestVoteResponse *, google::protobuf::RpcController *)> _Callback);

    /**
     * @brief 发起日志条目请求
     * @param request 请求消息体
     * @param callback 回调函数
    */
    void AppendEntries(int _Id, const AppendEntriesRequest * _Request,
                       std::function<void(int, const AppendEntriesResponse *, google::protobuf::RpcController *)> _Callback);

    /**
     * @brief 发起安装快照请求
     * @param request 请求消息体
     * @param callback 回调函数
    */
    void InstallSnapshot(int _Id, const InstallSnapshotRequest * _Request,
                         std::function<void(int, const InstallSnapshotResponse *, google::protobuf::RpcController *)> _Callback);

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
