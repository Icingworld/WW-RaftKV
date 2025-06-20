#include "RaftRpcClient.h"

#include <Memory.h>

namespace WW
{

RaftRpcClient::RaftRpcClient(std::shared_ptr<muduo::net::EventLoop> _Event_loop, const std::string & _Ip, const std::string & _Port)
    : _Event_loop(_Event_loop)
    , _Channel(std::make_unique<RaftRpcChannel>(_Event_loop, _Ip, _Port))
    , _Stub(std::make_unique<RaftService_Stub>(_Channel.get()))
{
}

RaftRpcClient::~RaftRpcClient()
{
    disconnect();
}

void RaftRpcClient::RequestVote(const RequestVoteRequest & _Request, RequestVoteCallback && _Callback)
{
    _Event_loop->runInLoop([this, request_copy = _Request, callback_move = std::move(_Callback)]() mutable {
        // 获取线程缓存的分配器
        thread_local MemoryPoolAllocator<RaftRpcController> controller_allocator;
        thread_local MemoryPoolAllocator<RequestVoteRequest> request_allocator;
        thread_local MemoryPoolAllocator<RequestVoteResponse> response_allocator;
        thread_local MemoryPoolAllocator<RequestVoteClosure> closure_allocator;

        // 创建控制器
        RaftRpcController * controller_ptr = controller_allocator.allocate(1);
        controller_allocator.construct(controller_ptr);
        std::unique_ptr<RaftRpcController, MemoryPoolDeleter<RaftRpcController>> controller(
            controller_ptr, MemoryPoolDeleter<RaftRpcController>()
        );
        // 创建请求
        RequestVoteRequest * request_ptr = request_allocator.allocate(1);
        request_allocator.construct(request_ptr, std::move(request_copy));
        std::unique_ptr<RequestVoteRequest, MemoryPoolDeleter<RequestVoteRequest>> request(
            request_ptr, MemoryPoolDeleter<RequestVoteRequest>()
        );
        // 创建响应
        RequestVoteResponse * response_ptr = response_allocator.allocate(1);
        response_allocator.construct(response_ptr);
        std::unique_ptr<RequestVoteResponse, MemoryPoolDeleter<RequestVoteResponse>> response(
            response_ptr, MemoryPoolDeleter<RequestVoteResponse>()
        );
        // 创建闭包
        RequestVoteClosure * closure_ptr = closure_allocator.allocate(1);
        closure_allocator.construct(
            closure_ptr,
            std::move(controller),
            std::move(request),
            std::move(response),
            std::move(callback_move)
        );

        // 发送请求
        _Stub->RequestVote(controller_ptr, request_ptr, response_ptr, closure_ptr);
    });
}

void RaftRpcClient::AppendEntries(const AppendEntriesRequest & _Request, AppendEntriesCallback && _Callback)
{
    _Event_loop->runInLoop([this, request_copy = _Request, callback_move = std::move(_Callback)]() mutable {
        // 获取线程缓存的分配器
        thread_local MemoryPoolAllocator<RaftRpcController> controller_allocator;
        thread_local MemoryPoolAllocator<AppendEntriesRequest> request_allocator;
        thread_local MemoryPoolAllocator<AppendEntriesResponse> response_allocator;
        thread_local MemoryPoolAllocator<AppendEntriesClosure> closure_allocator;

        // 创建控制器
        RaftRpcController * controller_ptr = controller_allocator.allocate(1);
        controller_allocator.construct(controller_ptr);
        std::unique_ptr<RaftRpcController, MemoryPoolDeleter<RaftRpcController>> controller(
            controller_ptr, MemoryPoolDeleter<RaftRpcController>()
        );
        // 创建请求
        AppendEntriesRequest * request_ptr = request_allocator.allocate(1);
        request_allocator.construct(request_ptr, std::move(request_copy));
        std::unique_ptr<AppendEntriesRequest, MemoryPoolDeleter<AppendEntriesRequest>> request(
            request_ptr, MemoryPoolDeleter<AppendEntriesRequest>()
        );
        // 创建响应
        AppendEntriesResponse * response_ptr = response_allocator.allocate(1);
        response_allocator.construct(response_ptr);
        std::unique_ptr<AppendEntriesResponse, MemoryPoolDeleter<AppendEntriesResponse>> response(
            response_ptr, MemoryPoolDeleter<AppendEntriesResponse>()
        );
        // 创建闭包
        AppendEntriesClosure * closure_ptr = closure_allocator.allocate(1);
        closure_allocator.construct(
            closure_ptr,
            std::move(controller),
            std::move(request),
            std::move(response),
            std::move(callback_move)
        );

        // 发送请求
        _Stub->AppendEntries(controller_ptr, request_ptr, response_ptr, closure_ptr);
    });
}

void RaftRpcClient::InstallSnapshot(const InstallSnapshotRequest & _Request, InstallSnapshotCallback && _Callback)
{
    _Event_loop->runInLoop([this, request_copy = _Request, callback_move = std::move(_Callback)]() mutable {
        // 获取线程缓存的分配器
        thread_local MemoryPoolAllocator<RaftRpcController> controller_allocator;
        thread_local MemoryPoolAllocator<InstallSnapshotRequest> request_allocator;
        thread_local MemoryPoolAllocator<InstallSnapshotResponse> response_allocator;
        thread_local MemoryPoolAllocator<InstallSnapshotClosure> closure_allocator;

        // 创建控制器
        RaftRpcController * controller_ptr = controller_allocator.allocate(1);
        controller_allocator.construct(controller_ptr);
        std::unique_ptr<RaftRpcController, MemoryPoolDeleter<RaftRpcController>> controller(
            controller_ptr, MemoryPoolDeleter<RaftRpcController>()
        );
        // 创建请求
        InstallSnapshotRequest * request_ptr = request_allocator.allocate(1);
        request_allocator.construct(request_ptr, std::move(request_copy));
        std::unique_ptr<InstallSnapshotRequest, MemoryPoolDeleter<InstallSnapshotRequest>> request(
            request_ptr, MemoryPoolDeleter<InstallSnapshotRequest>()
        );
        // 创建响应
        InstallSnapshotResponse * response_ptr = response_allocator.allocate(1);
        response_allocator.construct(response_ptr);
        std::unique_ptr<InstallSnapshotResponse, MemoryPoolDeleter<InstallSnapshotResponse>> response(
            response_ptr, MemoryPoolDeleter<InstallSnapshotResponse>()
        );
        // 创建闭包
        InstallSnapshotClosure * closure_ptr = closure_allocator.allocate(1);
        closure_allocator.construct(
            closure_ptr,
            std::move(controller),
            std::move(request),
            std::move(response),
            std::move(callback_move)
        );

        // 发送请求
        _Stub->InstallSnapshot(controller_ptr, request_ptr, response_ptr, closure_ptr);
    });
}

void RaftRpcClient::connect()
{
    _Channel->connect();
}

void RaftRpcClient::disconnect()
{
    _Channel->disconnect();
}

} // namespace WW
