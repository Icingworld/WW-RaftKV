#pragma once

#include <Raft.pb.h>
#include <RaftOperation.pb.h>

namespace WW
{

class RaftClerk;

/**
 * @brief 服务实例
 * @details 用于将服务与 Raft 算法本身分离
*/
class RaftRpcServiceImpl : public RaftService
{
private:
    RaftClerk * _Raft_clerk;

public:
    explicit RaftRpcServiceImpl(RaftClerk * _Raft_clerk = nullptr);

public:
    void RequestVote(google::protobuf::RpcController * _Controller,
                     const RequestVoteRequest * _Request,
                     RequestVoteResponse * _Response,
                     google::protobuf::Closure * _Done) override;

    void AppendEntries(google::protobuf::RpcController * _Controller,
                       const AppendEntriesRequest * _Request,
                       AppendEntriesResponse * _Response,
                       google::protobuf::Closure * _Done) override;

    void setRaftClerk(RaftClerk * _Raft_clerk);
};

/**
 * @brief kvstore 操作服务
*/
class RaftOperationServiceImpl : public RaftOperationService
{
private:
    RaftClerk * _Raft_clerk;

public:
    explicit RaftOperationServiceImpl(RaftClerk * _Raft_clerk = nullptr);

public:
    void OperateRaft(google::protobuf::RpcController * _Controller,
                     const RaftOperationRequest * _Request,
                     RaftOperationResponse * _Response,
                     google::protobuf::Closure * _Done) override;

    void setRaftClerk(RaftClerk * _Raft_clerk);
};

} // namespace WW