#include "RaftRpcClient.h"

#include <memory>

#include <RaftRpcController.h>

namespace WW
{

RaftRpcClient::RaftRpcClient(muduo::net::EventLoop * _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Event_loop(_Event_loop)
    , _Ip(_Ip)
    , _Port(_Port)
    , _Stub(nullptr)
    , _Channel(nullptr)
{
}

RaftRpcClient::~RaftRpcClient()
{
    disconnect();
}

void RaftRpcClient::RequestVote(const RequestVoteRequest * _Request, RequestVoteCallback _Callback)
{
    RaftRpcController * controller = new RaftRpcController();
    RequestVoteResponse * response = new RequestVoteResponse();
    RequestVoteClosure * closure = new RequestVoteClosure(controller, _Request, response, _Callback);
    _Stub->RequestVote(controller, _Request, response, closure);
}

void RaftRpcClient::AppendEntries(const AppendEntriesRequest * _Request, AppendEntriesCallback _Callback)
{
    RaftRpcController * controller = new RaftRpcController();
    AppendEntriesResponse * response = new AppendEntriesResponse();
    AppendEntriesClosure * closure = new AppendEntriesClosure(controller, _Request, response, _Callback);
    _Stub->AppendEntries(controller, _Request, response, closure);
}

void RaftRpcClient::InstallSnapshot(const InstallSnapshotRequest * _Request, InstallSnapshotCallback _Callback)
{
    RaftRpcController * controller = new RaftRpcController();
    InstallSnapshotResponse * response = new InstallSnapshotResponse();
    InstallSnapshotClosure * closure = new InstallSnapshotClosure(controller, _Request, response, _Callback);
    _Stub->InstallSnapshot(controller, _Request, response, closure);
}

void RaftRpcClient::connect()
{
    if (_Channel == nullptr) {
        _Channel = std::unique_ptr<RaftRpcChannel>(
            new RaftRpcChannel(_Event_loop, _Ip, _Port)
        );

        _Stub = std::unique_ptr<RaftService_Stub>(
            new RaftService_Stub(_Channel.get())
        );
    }

    _Channel->connect();
}

void RaftRpcClient::disconnect()
{
    if (_Channel != nullptr) {
        _Channel->disconnect();
    }
}

} // namespace WW
