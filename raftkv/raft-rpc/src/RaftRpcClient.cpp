#include "RaftRpcClient.h"

#include <memory>

#include <RaftRpcController.h>
#include <RaftRpcClosure.h>

namespace WW
{

RaftRpcClient::RaftRpcClient(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
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

void RaftRpcClient::RequestVote(const RequestVoteRequest * _Request,
                                std::function<void(const RequestVoteResponse *, google::protobuf::RpcController *)> _Callback)
{
    RaftRpcController * controller = new RaftRpcController();
    RequestVoteResponse * response = new RequestVoteResponse();
    RaftRpcClientClosure1<RequestVoteResponse> * closure = new RaftRpcClientClosure1<RequestVoteResponse>(controller, response, _Callback);
    _Stub->RequestVote(controller, _Request, response, closure);
}

void RaftRpcClient::AppendEntries(int _Id, const AppendEntriesRequest * _Request,
                                  std::function<void(int, const AppendEntriesResponse *, google::protobuf::RpcController *)> _Callback)
{
    RaftRpcController * controller = new RaftRpcController();
    AppendEntriesResponse * response = new AppendEntriesResponse();
    RaftRpcClientClosure2<AppendEntriesResponse> * closure = new RaftRpcClientClosure2<AppendEntriesResponse>(_Id, controller, response, _Callback);
    _Stub->AppendEntries(controller, _Request, response, closure);
}

void RaftRpcClient::InstallSnapshot(int _Id, const InstallSnapshotRequest * _Request,
                                    std::function<void(int, const InstallSnapshotResponse *, google::protobuf::RpcController *)> _Callback)
{
    RaftRpcController * controller = new RaftRpcController();
    InstallSnapshotResponse * response = new InstallSnapshotResponse();
    RaftRpcClientClosure2<InstallSnapshotResponse> * closure = new RaftRpcClientClosure2<InstallSnapshotResponse>(_Id, controller, response, _Callback);
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
