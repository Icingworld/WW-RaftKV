#include <string>
#include <iostream>
#include <memory>

#include <client_channel.h>
#include <RaftOperation.pb.h>
#include <RaftRpcClosure.h>
#include <muduo/base/Logging.h>

void ParseResponse(WW::RaftOperationService_Stub* stub, const WW::RaftOperationRequest& request, const WW::RaftOperationResponse& response)
{
    bool success = response.success();
    std::string value = response.value();
    bool is_leader = response.is_leader();
    std::string leader_address = response.leader_address();

    if (success) {
        std::cout << value << std::endl;
        return;
    }

    if (is_leader) {
        std::cerr << "something went wrong" << std::endl;
        return;
    }

    std::string ip;
    std::string port;
    size_t pos = leader_address.find(":");

    if (pos != std::string::npos) {
        ip = leader_address.substr(0, pos);
        port = leader_address.substr(pos + 1);
        std::cout << "leader is at: " << ip << ":" << port << std::endl;
    } else {
        std::cerr << "invalid leader address format" << std::endl;
    }
}

void SendOperationCommand(WW::RaftOperationService_Stub* stub, std::shared_ptr<WW::RaftOperationRequest> request, muduo::net::EventLoop* loop)
{
    auto response = std::make_shared<WW::RaftOperationResponse>();

    google::protobuf::Closure* done = new WW::RaftLambdaClosure([=]() {
        ParseResponse(stub, *request, *response);

        loop->queueInLoop([loop]() {
            loop->quit();
        });
    });

    stub->OperateRaft(nullptr, request.get(), response.get(), done);
}

int main(int argc, char** argv)
{
    auto request = std::make_shared<WW::RaftOperationRequest>();

    if (argc > 1) {
        std::string operation = argv[1];
        if (operation == "put") {
            request->set_type(WW::CommandType::PUT);
        } else if (operation == "update") {
            request->set_type(WW::CommandType::UPDATE);
        } else if (operation == "remove") {
            request->set_type(WW::CommandType::REMOVE);
        } else if (operation == "get") {
            request->set_type(WW::CommandType::GET);
        } else {
            std::cerr << "invalid operation!" << std::endl;
            return 1;
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
        return 1;
    }

    muduo::Logger::setLogLevel(muduo::Logger::LogLevel::ERROR);

    std::string ip = "127.0.0.1";
    std::string port = "4397";

    muduo::net::EventLoop event_loop;

    ClientChannel channel(&event_loop, ip, port);
    WW::RaftOperationService_Stub stub(&channel);

    channel.connect();

    // runAfter 保证发送操作发生在连接建立之后
    event_loop.runAfter(0.1, [&stub, request, &event_loop]() {
        SendOperationCommand(&stub, request, &event_loop);
    });

    event_loop.loop();

    return 0;
}
