#include "RaftRpcClient.h"

#include <memory>

#include <RaftRpcController.h>

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

void RaftRpcClient::RequestVote(std::unique_ptr<RequestVoteRequest> _Request, RequestVoteCallback _Callback)
{
    // 控制器
    std::unique_ptr<RaftRpcController> controller = std::unique_ptr<RaftRpcController>(
        new RaftRpcController()
    );
    // 响应
    std::unique_ptr<RequestVoteResponse> response = std::unique_ptr<RequestVoteResponse>(
        new RequestVoteResponse()
    );

    // 取出指针
    RaftRpcController * controller_ptr = controller.get();
    RequestVoteRequest * request_ptr = _Request.get();
    RequestVoteResponse * response_ptr = response.get();

    // 闭包
    RequestVoteClosure * closure = new RequestVoteClosure(
        std::move(controller), std::move(_Request), std::move(response), std::move(_Callback)
    );

    _Stub->RequestVote(controller_ptr, request_ptr, response_ptr, closure);
}

void RaftRpcClient::AppendEntries(std::unique_ptr<AppendEntriesRequest> _Request, AppendEntriesCallback _Callback)
{
    // 控制器
    std::unique_ptr<RaftRpcController> controller = std::unique_ptr<RaftRpcController>(
        new RaftRpcController()
    );
    // 响应
    std::unique_ptr<AppendEntriesResponse> response = std::unique_ptr<AppendEntriesResponse>(
        new AppendEntriesResponse()
    );

    // 取出指针
    RaftRpcController * controller_ptr = controller.get();
    AppendEntriesRequest * request_ptr = _Request.get();
    AppendEntriesResponse * response_ptr = response.get();

    // 闭包
    AppendEntriesClosure * closure = new AppendEntriesClosure(
        std::move(controller), std::move(_Request), std::move(response), std::move(_Callback)
    );

    _Stub->AppendEntries(controller_ptr, request_ptr, response_ptr, closure);
}

void RaftRpcClient::InstallSnapshot(std::unique_ptr<InstallSnapshotRequest> _Request, InstallSnapshotCallback _Callback)
{
    // 控制器
    std::unique_ptr<RaftRpcController> controller = std::unique_ptr<RaftRpcController>(
        new RaftRpcController()
    );
    // 响应
    std::unique_ptr<InstallSnapshotResponse> response = std::unique_ptr<InstallSnapshotResponse>(
        new InstallSnapshotResponse()
    );

    // 取出指针
    RaftRpcController * controller_ptr = controller.get();
    InstallSnapshotRequest * request_ptr = _Request.get();
    InstallSnapshotResponse * response_ptr = response.get();

    // 闭包
    InstallSnapshotClosure * closure = new InstallSnapshotClosure(
        std::move(controller), std::move(_Request), std::move(response), std::move(_Callback)
    );

    _Stub->InstallSnapshot(controller_ptr, request_ptr, response_ptr, closure);
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
