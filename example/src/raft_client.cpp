#include <string>
#include <iostream>
#include <future>

#include <client_channel.h>
#include <RaftOperation.pb.h>
#include <RaftRpcClosure.h>

static int count = 0;
std::promise<void> done_signal;

void ParseResponse(const WW::RaftOperationRequest & request, const WW::RaftOperationResponse & response);

void SendOperationCommand(std::string & ip, std::string & port, const WW::RaftOperationRequest & request)
{
    ClientChannel channel(ip, port);
    WW::RaftOperationService_Stub stub(&channel);

    WW::RaftOperationResponse response;

    google::protobuf::Closure * done = new WW::RaftLambdaClosure([&request, &response]() {
        ParseResponse(request, response);
    });

    stub.OperateRaft(nullptr, &request, &response, done);
}

void ParseResponse(const WW::RaftOperationRequest & request, const WW::RaftOperationResponse & response)
{
    bool success = response.success();
    std::string value = response.value();
    bool is_leader = response.is_leader();
    std::string leader_address = response.leader_address();

    if (success) {
        std::cout << value << std::endl;
        done_signal.set_value();
        return;
    }

    if (is_leader) {
        std::cerr << "something went wrong" << std::endl;
        done_signal.set_value();
        return;
    }

    ++count;
    if (count > 2) {
        done_signal.set_value();
        return;
    }

    std::string ip;
    std::string port;
    size_t pos = leader_address.find(":");
    
    if (pos != std::string::npos) {
        ip = response.leader_address().substr(0, pos);
        port = response.leader_address().substr(pos + 1);
    } else {
        std::cerr << "invalid leader address format" << std::endl;
        done_signal.set_value();
        return;
    }

    SendOperationCommand(ip, port, request);
}

int main(int argc, char ** argv)
{
    WW::RaftOperationRequest request;

    if (argc > 1) {
        std::string operation = argv[1];
        if (operation == "put") {
            request.set_type(WW::CommandType::PUT);
        } else if (operation == "update") {
            request.set_type(WW::CommandType::UPDATE);
        } else if (operation == "remove") {
            request.set_type(WW::CommandType::REMOVE);
        } else if (operation == "get") {
            request.set_type(WW::CommandType::GET);
        } else {
            std::cerr << "invalid operation!" << std::endl;
        }
    }

    if (argc > 2) {
        std::string key = argv[2];
        request.set_key(key);
    }

    if (argc > 3) {
        std::string value = argv[3];
        request.set_value(value);
    }

    if (argc > 4) {
        std::cerr << "invalid paramater!" << std::endl;
    }

    std::string ip = "127.0.0.1";
    std::string port = "4397";
    std::string ret = "";

    SendOperationCommand(ip, port, request);
    done_signal.get_future().wait();
}
