#include "RaftClerk.h"

#include <RaftPeer.h>
#include <RaftRpcClient.h>
#include <RaftLogger.h>
#include <muduo/base/Logging.h>

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

    // 设置 muduo 日志等级
    muduo::Logger::setLogLevel(muduo::Logger::LogLevel::ERROR);
}

RaftClerk::~RaftClerk()
{
    delete _Raft;
    delete _Server;
}

void RaftClerk::run()
{
    // 运行客户端线程
    _Running.store(true);
    _Client_thread = std::thread(&RaftClerk::_ClientWorking, this);

    // 运行服务端
    DEBUG("server running.");
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
    DEBUG("client thread delaying");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    DEBUG("client thread running");

    while (_Running.load()) {
        // 推进 Raft 状态
        std::unique_lock<std::mutex> lock(_Mutex);
        _Raft->tick(_Timeout);

        // 读取 Raft 输出信息并处理
        const std::vector<RaftMessage> & messages = _Raft->readInnerMessage();
        lock.unlock();

        for (const RaftMessage & message : messages) {
            _HandleRaftMessageOut(message);
        }

        lock.lock();
        _Raft->clearInnerMessage();
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
        const RaftPeerNet & peer = _Peers[_Message.to];

        // 创建一个 RpcClient
        RaftRpcClient client(peer.getIp(), peer.getPort());

        // 发送 Rpc 报文
        DEBUG("send vote request to node: %d", _Message.to);
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
    message.type = RaftMessage::MessageType::HeartbeatResponse;
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
    // 取出请求中的消息
    TermId other_term = _Request.term();
    NodeId other_id = _Request.candidate_id();
    LogIndex other_last_log_index = _Request.last_log_index();
    LogIndex other_last_log_term = _Request.last_log_term();

    // 构造消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::RequestVoteRequest;
    message.from = other_id;
    message.term = other_term;
    message.index = other_last_log_index;
    message.log_term = other_last_log_term;

    // 传入 Raft
    std::unique_lock<std::mutex> lock(_Mutex);
    _Raft->step(message);

    // 取出当此响应
    const RaftMessage & out_message = _Raft->readOutterMessage();
    lock.unlock();

    // 读取消息中的信息
    TermId this_term = out_message.term;
    bool this_reject = out_message.reject;

    // 设置响应
    _Response.set_term(this_term);
    _Response.set_voted(!this_reject);
}

void RaftClerk::_HandleRequestVoteResponse(const RequestVoteResponse & _Response)
{
    DEBUG("get vote response");
    // 取出响应中的信息
    TermId other_term = _Response.term();
    bool voted = _Response.voted();

    // 构造消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::RequestVoteResponse;
    message.term = other_term;
    message.reject = !voted;

    // 传递给 Raft 状态机
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_HandleAppendEntriesRequest(const AppendEntriesRequest & _Request, AppendEntriesResponse & _Response)
{
    // 取出响应中的信息
    TermId other_term = _Request.term();
    NodeId other_id = _Request.leader_id();
    LogIndex other_prev_log_index = _Request.prev_log_index();
    LogIndex other_prev_log_term = _Request.prev_log_term();
    const auto & other_entries = _Request.entry();
    LogIndex other_commit = _Request.leader_commit();

    // 构造消息
    RaftMessage message;

    // 判断条目是否为空
    if (other_entries.empty()) {
        // 是心跳包
        message.type = RaftMessage::MessageType::HeartbeatRequest;
    } else {
        message.type = RaftMessage::MessageType::AppendEntriesRequest;
        for (const auto & entry : other_entries) {
            message.entries.emplace_back(entry.term(), entry.command());
        }
    }

    message.term = other_term;
    message.from = other_id;
    message.index = other_prev_log_index;
    message.log_term = other_prev_log_term;
    message.commit = other_commit;

    // 传入 Raft
    std::unique_lock<std::mutex> lock(_Mutex);
    _Raft->step(message);

    // 取出当此响应
    const RaftMessage & out_message = _Raft->readOutterMessage();
    lock.unlock();

    // 读取消息中的信息
    TermId this_term = out_message.term;
    bool this_reject = out_message.reject;
    LogIndex this_index = out_message.index;

    // 设置响应
    _Response.set_term(this_term);
    _Response.set_success(!this_reject);
    _Response.set_index(this_index);
}

void RaftClerk::_HandleAppendEntriesResponse(NodeId _Id, const AppendEntriesResponse & _Response)
{
    
}

} // namespace WW
