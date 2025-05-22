#include "RaftClient.h"

#include <RaftClosure.h>

namespace WW
{

RaftClient::RaftClient(const std::string & _Ip, const std::string & _Port)
    : _Stub(nullptr)
    , _Channel(nullptr)
{
    _Channel = new RaftChannel(_Ip, _Port);
    _Stub = new RaftService_Stub(_Channel);
}

RaftClient::~RaftClient()
{
    delete _Channel;
    delete _Stub;
}

void RaftClient::RequestVote(const RequestVoteRequest & _Request, std::function<void(const RequestVoteResponse &)> _Callback)
{
    RequestVoteResponse * response = new RequestVoteResponse();
    RaftClosure<RequestVoteResponse> * closure = new RaftClosure<RequestVoteResponse>(response, _Callback);
    _Stub->RequestVote(nullptr, &_Request, response, closure);
}

void RaftClient::AppendEntries(const AppendEntriesRequest & _Request, std::function<void(const AppendEntriesResponse &)> _Callback)
{
    AppendEntriesResponse * response = new AppendEntriesResponse();
    RaftClosure<AppendEntriesResponse> * closure = new RaftClosure<AppendEntriesResponse>(response, _Callback);
    _Stub->AppendEntries(nullptr, &_Request, response, closure);
}

} // namespace WW
