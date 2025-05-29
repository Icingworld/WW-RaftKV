#pragma once

#include <functional>
#include <thread>

#include <RaftCommon.h>
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
    RaftService_Stub * _Stub;       // 客户端
    RaftRpcChannel * _Channel;      // 通道

public:
    RaftRpcClient(muduo::net::EventLoop * _Loop, const std::string & _Ip, const std::string & _Port);

    ~RaftRpcClient();

public:
    /**
     * @brief 发起投票请求
     * @param request 请求消息体
     * @param callback 回调函数
    */
    void RequestVote(const RequestVoteRequest & _Request, std::function<void(const RequestVoteResponse &)> _Callback);

    /**
     * @brief 发起日志条目请求
     * @param request 请求消息体
     * @param callback 回调函数
    */
    void AppendEntries(const AppendEntriesRequest & _Request, NodeId _To, std::function<void(NodeId, const AppendEntriesResponse &)> _Callback);

    /**
     * @brief 连接 Raft 服务端
    */
    void connect();
};

} // namespace WW
