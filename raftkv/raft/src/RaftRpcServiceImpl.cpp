#include "RaftRpcServiceImpl.h"

#include <RaftClerk.h>

namespace WW
{

RaftRpcServiceImpl::RaftRpcServiceImpl(RaftClerk * _Raft_clerk)
    : _Raft_clerk(_Raft_clerk)
{
}

void RaftRpcServiceImpl::RequestVote(google::protobuf::RpcController * _Controller,
                                  const RequestVoteRequest * _Request,
                                  RequestVoteResponse * _Response,
                                  google::protobuf::Closure * _Done)
{
    _Raft_clerk->_HandleRequestVoteRequest(*_Request, *_Response);

    _Done->Run();
}

void RaftRpcServiceImpl::AppendEntries(google::protobuf::RpcController * _Controller,
                                    const AppendEntriesRequest * _Request,
                                    AppendEntriesResponse * _Response,
                                    google::protobuf::Closure * _Done)
{
    _Raft_clerk->_HandleAppendEntriesRequest(*_Request, *_Response);
    
    _Done->Run();
}

void RaftRpcServiceImpl::setRaftClerk(RaftClerk * _Raft_clerk)
{
    this->_Raft_clerk = _Raft_clerk;
}

} // namespace WW
