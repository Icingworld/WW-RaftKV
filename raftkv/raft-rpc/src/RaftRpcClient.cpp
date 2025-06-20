#include "RaftRpcClient.h"

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

void RaftRpcClient::RequestVote(const RequestVoteRequest & _Request, RequestVoteCallback && _Callback)
{
    _Event_loop->runInLoop([this, request_copy = _Request, callback_move = std::move(_Callback)]() mutable {
        // 创建控制器
        std::unique_ptr<RaftRpcController> controller = std::make_unique<RaftRpcController>();
        // 创建请求
        std::unique_ptr<RequestVoteRequest> request = std::make_unique<RequestVoteRequest>(request_copy);
        // 创建响应
        std::unique_ptr<RequestVoteResponse> response = std::make_unique<RequestVoteResponse>();
        // 创建闭包
        RequestVoteClosure * closure = new RequestVoteClosure(
            std::move(controller), std::move(request), std::move(response), std::move(callback_move)
        );

        // 发送请求
        _Stub->RequestVote(closure->getController(), closure->getRequest(), closure->getResponse(), closure);
    });
}

void RaftRpcClient::AppendEntries(const AppendEntriesRequest & _Request, AppendEntriesCallback && _Callback)
{
    _Event_loop->runInLoop([this, request_copy = _Request, callback = std::move(_Callback)]() mutable {
        // 创建控制器
        std::unique_ptr<RaftRpcController> controller = std::make_unique<RaftRpcController>();
        // 重新获得请求的所有权
        std::unique_ptr<AppendEntriesRequest> request = std::make_unique<AppendEntriesRequest>(request_copy);
        // 创建响应
        std::unique_ptr<AppendEntriesResponse> response = std::make_unique<AppendEntriesResponse>();
        // 创建闭包
        AppendEntriesClosure * closure = new AppendEntriesClosure(
            std::move(controller), std::move(request), std::move(response), std::move(callback)
        );

        // 发送请求
        _Stub->AppendEntries(closure->getController(), closure->getRequest(), closure->getResponse(), closure);
    });
}

void RaftRpcClient::InstallSnapshot(const InstallSnapshotRequest & _Request, InstallSnapshotCallback && _Callback)
{
    _Event_loop->runInLoop([this, request_copy = _Request, callback = std::move(_Callback)]() mutable {
        // 创建控制器
        std::unique_ptr<RaftRpcController> controller = std::make_unique<RaftRpcController>();
        // 重新获得请求的所有权
        std::unique_ptr<InstallSnapshotRequest> request = std::make_unique<InstallSnapshotRequest>(request_copy);
        // 创建响应
        std::unique_ptr<InstallSnapshotResponse> response = std::make_unique<InstallSnapshotResponse>();
        // 创建闭包
        InstallSnapshotClosure * closure = new InstallSnapshotClosure(
            std::move(controller), std::move(request), std::move(response), std::move(callback)
        );

        // 发送请求
        _Stub->InstallSnapshot(closure->getController(), closure->getRequest(), closure->getResponse(), closure);
    });
}

void RaftRpcClient::connect()
{
    if (_Channel == nullptr) {
        _Channel = std::make_unique<RaftRpcChannel>(_Event_loop, _Ip, _Port);
        _Stub = std::make_unique<RaftService_Stub>(_Channel.get());
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
