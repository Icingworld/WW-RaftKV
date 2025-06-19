#include "RaftRpcService.h"

namespace WW
{

void RaftRpcServiceImpl::RequestVote(google::protobuf::RpcController * _Controller,
                                     const RequestVoteRequest * _Request,
                                     RequestVoteResponse * _Response,
                                     google::protobuf::Closure * _Done)
{
    _RequestVoteCallback(_Request, _Done);
}

void RaftRpcServiceImpl::AppendEntries(google::protobuf::RpcController * _Controller,
                                       const AppendEntriesRequest * _Request,
                                       AppendEntriesResponse * _Response,
                                       google::protobuf::Closure * _Done)
{
    _AppendEntriesCallback(_Request, _Done);
}

void RaftRpcServiceImpl::InstallSnapshot(google::protobuf::RpcController * _Controller,
                                         const InstallSnapshotRequest * _Request,
                                         InstallSnapshotResponse * _Response,
                                         google::protobuf::Closure * _Done)
{
    _InstallSnapshotCallback(_Request, _Done);
}

void RaftRpcServiceImpl::registerRequestVoteCallback(RequestVoteCallback && _Callback)
{
    _RequestVoteCallback = std::move(_Callback);
}

void RaftRpcServiceImpl::registerAppendEntriesCallback(AppendEntriesCallback && _Callback)
{
    _AppendEntriesCallback = std::move(_Callback);
}

void RaftRpcServiceImpl::registerInstallSnapshotCallback(InstallSnapshotCallback && _Callback)
{
    _InstallSnapshotCallback = std::move(_Callback);
}

void KVOperationServiceImpl::Execute(google::protobuf::RpcController * _Controller,
                                     const KVOperationRequest * _Request,
                                     KVOperationResponse * _Response,
                                     google::protobuf::Closure * _Done)
{
    _ExecuteCallback(_Request, _Done);
}

void KVOperationServiceImpl::registerExecuteCallback(ExecuteCallback && _Callback)
{
    _ExecuteCallback = std::move(_Callback);
}

} // namespace WW
