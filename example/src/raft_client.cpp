#include <string>
#include <iostream>
#include <memory>

#include <uuid.h>
#include <client_channel.h>
#include <KVOperation.pb.h>
#include <RaftRpcController.h>
#include <RaftRpcClosure.h>
#include <muduo/base/Logging.h>

void ParseResponse(const WW::KVOperationResponse * _Response, google::protobuf::RpcController * _Controller)
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

void SendKVOperationCommand(WW::KVOperationService_Stub * stub, const WW::KVOperationRequest * request, std::shared_ptr<muduo::net::EventLoop> loop)
{
    WW::RaftRpcController * controller = new WW::RaftRpcController();
    WW::KVOperationResponse * response = new WW::KVOperationResponse();
    WW::RaftRpcClientClosure<WW::KVOperationRequest, WW::KVOperationResponse> * closure = new WW::RaftRpcClientClosure<WW::KVOperationRequest, WW::KVOperationResponse>(controller, request, response, std::bind(
        ParseResponse, std::placeholders::_1, std::placeholders::_2
    ));
    stub->Execute(controller, request, response, closure);
}

int main(int argc, char ** argv)
{
    WW::KVOperationRequest * request = new WW::KVOperationRequest();

    // 生成 UUID
    UUID uuid;
    WW::Meta * meta = request->mutable_meta();
    meta->set_uuid(uuid.toString());

    if (argc > 1) {
        std::string operation = argv[1];
        if (operation == "put") {
            request->set_type(WW::OperationType::PUT);
        } else if (operation == "update") {
            request->set_type(WW::OperationType::UPDATE);
        } else if (operation == "delete") {
            request->set_type(WW::OperationType::DELETE);
        } else if (operation == "get") {
            request->set_type(WW::OperationType::GET);
        } else {
            std::cerr << "invalid operation!" << std::endl;
            return -1;
        }
    }

    if (argc > 2) {
        request->set_key(argv[2]);
    }

    if (argc > 3) {
        request->set_value(argv[3]);
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

    channel.connect();

    // runAfter 保证发送操作发生在连接建立之后
    event_loop->runAfter(0.1, [&stub, request, &event_loop]() {
        SendKVOperationCommand(&stub, request, event_loop);
    });

    event_loop->loop();

    return 0;
}