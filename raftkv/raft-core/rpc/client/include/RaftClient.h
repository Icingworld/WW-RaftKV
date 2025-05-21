#pragma once

#include <string>

#include <raft.pb.h>

namespace WW
{

/**
 * @brief Raft 客户端
*/
class RaftClient
{
private:
    RaftService_Stub * _Stub;       // 客户端

public:
    RaftClient(const std::string & _Ip, const std::string & _Port);

    ~RaftClient();

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
    void AppendEntries(const AppendEntriesRequest & _Request, std::function<void(const AppendEntriesResponse &)> _Callback);
};

} // namespace WW
