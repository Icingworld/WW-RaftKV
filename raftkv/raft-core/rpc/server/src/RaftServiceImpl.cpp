#include "RaftServiceImpl.h"

#include <Raft.h>

namespace WW
{

RaftServiceImpl::RaftServiceImpl(Raft * _Raft)
    : _Raft(_Raft)
{
}

void RaftServiceImpl::RequestVote(google::protobuf::RpcController * controller,
                                  const RequestVoteRequest * request,
                                  RequestVoteResponse * response,
                                  google::protobuf::Closure * done)
{
    _Raft->_OnVoteRequest(*request, *response);

    if (done != nullptr) {
        done->Run();
    }
}

void RaftServiceImpl::AppendEntries(google::protobuf::RpcController * controller,
                                    const AppendEntriesRequest * request,
                                    AppendEntriesResponse * response,
                                    google::protobuf::Closure * done)
{
    _Raft->_OnAppendEntriesRequest(*request, *response);

    if (done != nullptr) {
        done->Run();
    }
}

void RaftServiceImpl::setRaft(Raft * _Raft)
{
    _Raft = _Raft;
}

} // namespace WW
