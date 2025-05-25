#include "RaftClerk.h"

#include <RaftPeer.h>
#include <RaftRpcClient.h>

namespace WW
{

RaftClerk::RaftClerk(NodeId _Id, const std::vector<RaftPeerNet> & _Peers)
    : _Raft(nullptr)
    , _Peers(_Peers)
{
    // 初始化 Raft
    std::vector<RaftPeer> peers;
    for (const RaftPeerNet & peer_net : _Peers) {
        peers.emplace_back(peer_net.getId());
    }
    _Raft = new Raft(_Id, peers);

    // 初始化服务端
    _Service.setRaftClerk(this);
    _Server = new RaftRpcServer(_Peers[_Id].getIp(), _Peers[_Id].getPort(), &_Service);
}

RaftClerk::~RaftClerk()
{
    delete _Raft;
}

void RaftClerk::run()
{
    // 运行客户端线程
    _Running.store(true);
    _Client_thread = std::thread(&RaftClerk::_ClientWorking, this);

    // 运行服务端
    _Server->run();
}

void RaftClerk::stop()
{
    // 关闭定时器线程
    _Running.store(false);
    if (_Client_thread.joinable()) {
        _Client_thread.join();
    }
}

void RaftClerk::_ClientWorking()
{
    // 等待服务端全部启动
    std::this_thread::sleep_for(std::chrono::seconds(10));

    while (_Running.load()) {
        // 推进 Raft 状态
        std::unique_lock<std::mutex> lock(_Mutex);
        _Raft->tick(_Timeout);

        // 读取 Raft 输出信息并处理
        const std::vector<RaftMessage> messages = _Raft->readReady();
        lock.unlock();

        for (const RaftMessage & message : messages) {
            _HandleRaftMessageOut(message);
        }

        lock.lock();
        _Raft->clearReady();
        lock.unlock();

        // 等待 _Timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(_Timeout));
    }
}

void RaftClerk::_HandleRaftMessageOut(const RaftMessage & _Message)
{
    switch (_Message.type) {
        case RaftMessage::MessageType::HeartbeatRequest:
            _SendHeartbeatRequest(_Message);
            break;
        case RaftMessage::MessageType::RequestVoteRequest:
            _SendRequestVoteRequest(_Message);
            break;
        case RaftMessage::MessageType::AppendEntriesRequest:
            _SendAppendEntriesRequest(_Message);
            break;
        default:
            break;
    }
}

void RaftClerk::_SendHeartbeatRequest(const RaftMessage & _Message)
{
    // 构造 Request 消息体
    AppendEntriesRequest heartbeat_request;
    heartbeat_request.set_term(_Message.term);
    heartbeat_request.set_leader_id(_Message.from);
    heartbeat_request.set_prev_log_index(_Message.index);
    heartbeat_request.set_prev_log_term(_Message.log_term);
    heartbeat_request.set_leader_commit(_Message.commit);

    // 创建一个线程来发送
    std::thread temp_thread([this, _Message, heartbeat_request]() {
        // 获取当前节点的信息
        const RaftPeerNet & peer = _Peers[_Message.from];

        // 创建一个 RpcClient
        RaftRpcClient client(peer.getIp(), peer.getPort());

        // 发送 Rpc 报文
        client.AppendEntries(heartbeat_request, peer.getId(), std::bind(&RaftClerk::_HandleHeartbeatResponse, this, std::placeholders::_1, std::placeholders::_2));
    });

    // 分离线程
    temp_thread.detach();
}

void RaftClerk::_SendRequestVoteRequest(const RaftMessage & _Message)
{
    // 构造 Request 消息体
    RequestVoteRequest vote_request;
    vote_request.set_term(_Message.term);
    vote_request.set_candidate_id(_Message.from);
    vote_request.set_last_log_index(_Message.index);
    vote_request.set_last_log_term(_Message.log_term);

    // 创建一个线程来发送
    std::thread temp_thread([this, _Message, vote_request]() {
        // 获取当前节点的信息
        const RaftPeerNet & peer = _Peers[_Message.from];

        // 创建一个 RpcClient
        RaftRpcClient client(peer.getIp(), peer.getPort());

        // 发送 Rpc 报文
        client.RequestVote(vote_request, std::bind(&RaftClerk::_HandleRequestVoteResponse, this, std::placeholders::_1));
    });

    // 分离线程
    temp_thread.detach();
}

void RaftClerk::_SendAppendEntriesRequest(const RaftMessage & _Message)
{
    
}

void RaftClerk::_HandleHeartbeatResponse(NodeId _Id, const AppendEntriesResponse & _Response)
{
    // 取出响应中的信息
    TermId other_term = _Response.term();
    NodeId other_id = _Id;
    bool other_success = _Response.success();
    LogIndex other_index = _Response.index();

    // 构造消息
    RaftMessage message;
    message.term = other_term;
    message.from = other_id;
    message.index = other_index;
    message.reject = !other_success;

    // 传递给 Raft 状态机
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_HandleRequestVoteRequest(const RequestVoteRequest & _Request, RequestVoteResponse & _Response)
{

}

void RaftClerk::_HandleRequestVoteResponse(const RequestVoteResponse & _Response)
{
    // 取出响应中的信息
    TermId other_term = _Response.term();
    bool voted = _Response.voted();

    // 构造消息
    RaftMessage message;
    message.term = other_term;
    message.reject = !voted;

    // 传递给 Raft 状态机
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_HandleAppendEntriesRequest(const AppendEntriesRequest & _Request, AppendEntriesResponse & _Response)
{

}

void RaftClerk::_HandleAppendEntriesResponse(NodeId _Id, const AppendEntriesResponse & _Response)
{
    
}

} // namespace WW
