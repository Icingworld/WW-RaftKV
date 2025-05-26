#include "RaftRpcClient.h"

#include <memory>

#include <RaftRpcClosure.h>

namespace WW
{

RaftRpcClient::RaftRpcClient(const std::string & _Ip, const std::string & _Port)
    : _Stub(nullptr)
    , _Channel(nullptr)
{
    _Channel = new RaftRpcChannel(_Ip, _Port);
    _Stub = new RaftService_Stub(_Channel);
}

RaftRpcClient::~RaftRpcClient()
{
    delete _Channel;
    delete _Stub;
}

void RaftRpcClient::RequestVote(const RequestVoteRequest & _Request, std::function<void(const RequestVoteResponse &)> _Callback)
{
    RequestVoteResponse * response = new RequestVoteResponse();
    RaftRpcClosure<RequestVoteResponse> * closure = new RaftRpcClosure<RequestVoteResponse>(response, _Callback);
    _Stub->RequestVote(nullptr, &_Request, response, closure);
}

void RaftRpcClient::AppendEntries(const AppendEntriesRequest & _Request, NodeId _To, std::function<void(NodeId, const AppendEntriesResponse &)> _Callback)
{
    AppendEntriesResponse * response = new AppendEntriesResponse();

    RaftRpcClosure<AppendEntriesResponse> * closure = new RaftRpcClosure<AppendEntriesResponse>(response, [_To, _Callback](const AppendEntriesResponse & _Response) {
        _Callback(_To, _Response);
    });
    _Stub->AppendEntries(nullptr, &_Request, response, closure);
}

} // namespace WW
