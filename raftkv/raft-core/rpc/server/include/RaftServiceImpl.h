#pragma once

#include <raft.pb.h>

namespace WW
{

class Raft;

/**
 * @brief 服务实例
 * @details 用于将服务与 Raft 算法本身分离
*/
class RaftServiceImpl : public RaftService
{
private:
    Raft * _Raft;

public:
    explicit RaftServiceImpl(Raft * _Raft = nullptr);

public:
    void RequestVote(google::protobuf::RpcController * controller,
                     const RequestVoteRequest * request,
                     RequestVoteResponse * response,
                     google::protobuf::Closure * done) override;

    void AppendEntries(google::protobuf::RpcController * controller,
                       const AppendEntriesRequest * request,
                       AppendEntriesResponse * response,
                       google::protobuf::Closure * done) override;

    void setRaft(Raft * _Raft);
};

} // namespace WW
