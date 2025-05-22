#pragma once

#include <string>

#include <RaftDispatcher.h>
#include <raft.pb.h>

namespace WW
{

/**
 * @brief Raft 服务端
*/
class RaftServer
{
private:
    RaftDispatcher * _Dispatcher;

public:
    RaftServer(const std::string & _Ip, const std::string & _Port, google::protobuf::Service * _Service = nullptr);

    ~RaftServer();

public:
    /**
     * @brief 启动服务端
    */
    void run();

    void registerService(google::protobuf::Service * _Service);
};

} // namespace WW
