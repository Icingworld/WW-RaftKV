#pragma once

#include <functional>

#include <Raft.pb.h>
#include <KVOperation.pb.h>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>

namespace WW
{

class RaftClerk;

/**
 * @brief Raft 服务实例
 * @details 用于将服务与 Raft 算法本身分离
*/
class RaftRpcServiceImpl : public RaftService
{
public:
    using RequestVoteCallback = std::function<void(const RequestVoteRequest *, google::protobuf::Closure *)>;
    using AppendEntriesCallback = std::function<void(const AppendEntriesRequest * _Request, google::protobuf::Closure *)>;
    using InstallSnapshotCallback = std::function<void(const InstallSnapshotRequest *, google::protobuf::Closure *)>;

private:
    RequestVoteCallback _RequestVoteCallback;
    AppendEntriesCallback _AppendEntriesCallback;
    InstallSnapshotCallback _InstallSnapshotCallback;

public:
    RaftRpcServiceImpl();

public:
    void RequestVote(google::protobuf::RpcController * _Controller,
                     const RequestVoteRequest * _Request,
                     RequestVoteResponse * _Response,
                     google::protobuf::Closure * _Done) override;

    void AppendEntries(google::protobuf::RpcController * _Controller,
                       const AppendEntriesRequest * _Request,
                       AppendEntriesResponse * _Response,
                       google::protobuf::Closure * _Done) override;

    void InstallSnapshot(google::protobuf::RpcController * _Controller,
                         const InstallSnapshotRequest * _Request,
                         InstallSnapshotResponse * _Response,
                         google::protobuf::Closure * _Done) override;

    void registerRequestVoteCallback(RequestVoteCallback _Callback);

    void registerAppendEntriesCallback(AppendEntriesCallback _Callback);

    void registerInstallSnapshotCallback(InstallSnapshotCallback _Callback);
};

/**
 * @brief KV 操作服务实例
*/
class KVOperationServiceImpl : public KVOperationService
{
public:
    using ExecuteCallback = std::function<void(const KVOperationRequest *, google::protobuf::Closure *)>;

private:
    ExecuteCallback _ExecuteCallback;

public:
    KVOperationServiceImpl();

public:
    void Execute(google::protobuf::RpcController * _Controller,
                 const KVOperationRequest * _Request,
                 KVOperationResponse * _Response,
                 google::protobuf::Closure * _Done) override;

    void registerExecuteCallback(ExecuteCallback _Callback);
};

} // namespace WW