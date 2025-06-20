#include <string>
#include <iostream>
#include <memory>

#include <uuid.h>
#include <client_channel.h>
#include <Memory.h>
#include <RaftRpcController.h>
#include <RaftRpcClosure.h>

#include <muduo/base/Logging.h>

using KVOperationClosure = WW::RaftRpcClientClosure<WW::RaftRpcController, WW::KVOperationRequest, WW::KVOperationResponse>;

void ParseResponse(const WW::KVOperationResponse * _Response, const google::protobuf::RpcController * _Controller)
{
    // 读取响应中的信息
    WW::KVOperationResponse::StatusCode status_code = _Response->status_code();

    switch (status_code) {
    case WW::KVOperationResponse_StatusCode_SUCCESS: {
        std::string value = _Response->payload();
        std::cout << "success: " << value << std::endl;
        break;
    }
    case WW::KVOperationResponse_StatusCode_CREATED: {
        std::cout << "success" << std::endl;
        break;
    }
    case WW::KVOperationResponse_StatusCode_REDIRECT: {
        std::string address = _Response->address();
        std::cout << "not leader, redirected to: " << address << std::endl;
        break;
    }
    case WW::KVOperationResponse_StatusCode_NOT_FOUND: {
        std::cout << "key not found" << std::endl;
        break;
    }
    case WW::KVOperationResponse_StatusCode_BAD_REQUEST: {
        std::cout << "bad request" << std::endl;
        break;
    }
    case WW::KVOperationResponse_StatusCode_INTERNAL_ERROR: {
        std::cout << "internal error, operation failed" << std::endl;
        break;
    }
    }

    exit(0);
}

void SendKVOperationCommand(WW::KVOperationService_Stub * stub, const WW::KVOperationRequest & request, std::shared_ptr<muduo::net::EventLoop> loop)
{
    loop->runInLoop([stub, request_copy = request]() {
        // 获取线程缓存的分配器
        thread_local WW::MemoryPoolAllocator<WW::RaftRpcController> controller_allocator;
        thread_local WW::MemoryPoolAllocator<WW::KVOperationRequest> request_allocator;
        thread_local WW::MemoryPoolAllocator<WW::KVOperationResponse> response_allocator;
        thread_local WW::MemoryPoolAllocator<KVOperationClosure> closure_allocator;

        // 创建控制器
        WW::RaftRpcController * controller_ptr = controller_allocator.allocate(1);
        controller_allocator.construct(controller_ptr);
        std::unique_ptr<WW::RaftRpcController, WW::MemoryPoolDeleter<WW::RaftRpcController>> controller(
            controller_ptr, WW::MemoryPoolDeleter<WW::RaftRpcController>()
        );
        // 创建请求
        WW::KVOperationRequest * request_ptr = request_allocator.allocate(1);
        request_allocator.construct(request_ptr, std::move(request_copy));
        std::unique_ptr<WW::KVOperationRequest, WW::MemoryPoolDeleter<WW::KVOperationRequest>> request(
            request_ptr, WW::MemoryPoolDeleter<WW::KVOperationRequest>()
        );
        // 创建响应
        WW::KVOperationResponse * response_ptr = response_allocator.allocate(1);
        response_allocator.construct(response_ptr);
        std::unique_ptr<WW::KVOperationResponse, WW::MemoryPoolDeleter<WW::KVOperationResponse>> response(
            response_ptr, WW::MemoryPoolDeleter<WW::KVOperationResponse>()
        );
        KVOperationClosure * closure_ptr = closure_allocator.allocate(1);
        closure_allocator.construct(
            closure_ptr,
            std::move(controller),
            std::move(request),
            std::move(response),
            std::bind(ParseResponse, std::placeholders::_1, std::placeholders::_2)
        );

        // 发送请求
        stub->Execute(controller_ptr, request_ptr, response_ptr, closure_ptr);
    });
}

int main(int argc, char ** argv)
{
    WW::KVOperationRequest request;

    // 生成 UUID
    UUID uuid;
    WW::Meta * meta = request.mutable_meta();
    meta->set_uuid(uuid.toString());

    if (argc > 1) {
        std::string operation = argv[1];
        if (operation == "put") {
            request.set_type(WW::OperationType::PUT);
        } else if (operation == "update") {
            request.set_type(WW::OperationType::UPDATE);
        } else if (operation == "delete") {
            request.set_type(WW::OperationType::DELETE);
        } else if (operation == "get") {
            request.set_type(WW::OperationType::GET);
        } else {
            std::cerr << "invalid operation!" << std::endl;
            return -1;
        }
    }

    if (argc > 2) {
        request.set_key(argv[2]);
    }

    if (argc > 3) {
        request.set_value(argv[3]);
    }

    if (argc > 4) {
        std::cerr << "invalid parameter!" << std::endl;
        return -1;
    }

    // 设置 muduo 日志等级
    muduo::Logger::setLogLevel(muduo::Logger::LogLevel::ERROR);

    // 默认为 node 0 的地址
    std::string ip = "127.0.0.1";
    std::string port = "4397";

    // 开启事件循环
    std::shared_ptr<muduo::net::EventLoop> event_loop = std::make_shared<muduo::net::EventLoop>();

    ClientChannel channel(event_loop, ip, port);
    WW::KVOperationService_Stub stub(&channel);

    // 注册连接回调
    channel.setConnectedCallback([&] {
        SendKVOperationCommand(&stub, std::move(request), event_loop);
    });

    // 连接服务端
    channel.connect();

    event_loop->loop();

    return 0;
}