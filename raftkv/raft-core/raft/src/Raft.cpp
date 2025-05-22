#include "Raft.h"

#include <RaftClient.h>

namespace WW
{

Raft::Raft(NodeId _Id, const std::vector<RaftPeer> & _Peers)
    : _Node(_Id)
    , _Peers(_Peers)
    , _Election_timeout_min(150)
    , _Election_timeout_max(300)
    , _Rng(std::random_device{}())
    , _Election_dist(_Election_timeout_min, _Election_timeout_max)
    , _Election_timeout(0)
    , _Running(false)
    , _Server(_Peers[_Id].getIp(), _Peers[_Id].getPort())
{
    _Election_timeout = _Election_dist(_Rng);

    // 将自己注册到 service
    _Service.setRaft(this);

    // 将服务注册到 server
    _Server.registerService(&_Service);
}

void Raft::run()
{
    _Running.store(true);
    _Client_thread = std::thread(&Raft::_ClientWorkingThread, this);

    _Server.run();
}

void Raft::stop()
{
    _Running.store(false);

    if (_Client_thread.joinable()) {
        _Client_thread.join();
    }
}

void Raft::_ClientWorkingThread()
{
    // 等待服务端全部启动
    std::this_thread::sleep_for(std::chrono::seconds(10));

    while (_Running.load()) {
        _Tick();

        // 等待一段时间 10ms
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Raft::_Tick()
{
    Timestamp now = std::chrono::steady_clock::now();

    // 根据身份判断定时器
    switch (_Node.getRole()) {
        case RaftNode::NodeRole::Follower:
        case RaftNode::NodeRole::Candidate:
            if (now - _Last_heartbeat_recv >= std::chrono::milliseconds(_Election_timeout)) {
                // Leader 失联，发起选举
                _StartElection();
            }
            break;
        case RaftNode::NodeRole::Leader:
            if (now - _Last_heartbeat_send >= std::chrono::milliseconds(_Heartbeat_timeout)) {
                // 到达时间，向其他节点发送心跳
                _SendHeartbeat();
            }
            break;
        default:
            break;
    }
}

void Raft::_StartElection()
{
    // 切换身份为 Candidate
    _Node.setRole(RaftNode::NodeRole::Candidate);

    // 将任期号增加1
    TermId new_term = _Node.getTerm() + 1;
    _Node.setTerm(new_term);

    // 将手中的一票投给自己
    _Node.setVotedFor(_Node.getId());
    _Node.setVoteCount(_Node.getVoteCount() + 1);

    // 构造投票请求
    RequestVoteRequest vote_request;
    vote_request.set_term(new_term);
    vote_request.set_candidate_id(_Node.getId());
    vote_request.set_last_log_index(_Node.getLastLogIndex());
    vote_request.set_last_log_term(_Node.getLastLogTerm());

    // 重置时间，防止重新发起选举
    _Last_heartbeat_recv = std::chrono::steady_clock::now();

    // 发送给其他节点
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == _Node.getId()) {
            continue;
        }

        // 创建一个异步 RPC 客户端
        RaftClient client(peer.getIp(), peer.getPort());
        // 发送请求并传入回调函数
        client.RequestVote(vote_request, std::bind(&Raft::_OnVoteResponse, this, std::placeholders::_1));
    }
}

void Raft::_SendHeartbeat()
{
    // 构造心跳请求
    AppendEntriesRequest heartbeat_request;
    heartbeat_request.set_term(_Node.getTerm());
    heartbeat_request.set_leader_id(_Node.getId());
    // 携带最后一条日志情况，用于确认同步
    heartbeat_request.set_prev_log_index(_Node.getLastLogIndex());
    heartbeat_request.set_prev_log_term(_Node.getLastLogTerm());
    // 对于心跳包，LogEntry 为空即可
    // TODO 暂时没有 commit 相关设计
    heartbeat_request.set_leader_commit(0);

    // 重置发送时间
    _Last_heartbeat_send = std::chrono::steady_clock::now();

    // 发送给其他节点
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == _Node.getId()) {
            continue;
        }

        // 创建一个异步 RPC 客户端
        RaftClient client(peer.getIp(), peer.getPort());
        // 发送请求并传入回调函数
        client.AppendEntries(heartbeat_request, std::bind(&Raft::_OnAppendEntriesResponse, this, std::placeholders::_1));
    }
}

void Raft::_OnVoteResponse(const RequestVoteResponse & _Response)
{
    std::lock_guard<std::mutex> lock(_Mutex);

    if (!_Running.load() || _Node.getRole() != RaftNode::NodeRole::Candidate) {
        // 停止运行或自己在其他线程已经退出选举
        return;
    }

    // 获取响应数据
    TermId other_term = _Response.term();
    bool other_voted = _Response.voted();

    if (other_term > _Node.getTerm()) {
        // 任期号落后，退出选举
        _Node.setTerm(other_term);
        _Node.setRole(RaftNode::NodeRole::Follower);
        _Node.setVotedFor(-1);
        return;
    }

    if (other_voted) {
        // 增加一票
        _Node.setVoteCount(_Node.getVoteCount() + 1);

        // 判断自己是否胜选
        if (_Node.getVoteCount() > (_Peers.size() / 2)) {
            // 赢得选举
            _Node.setRole(RaftNode::NodeRole::Leader);
            // 发送心跳宣布自己当选 Leader
            _SendHeartbeat();
        }
    }
}

void Raft::_OnAppendEntriesResponse(const AppendEntriesResponse & _Response)
{
    // 获取响应信息
    TermId other_term = _Response.term();
    bool other_success = _Response.success();

    std::lock_guard<std::mutex> lock(_Mutex);

    if (other_term > _Node.getTerm()) {
        // 任期号落后，出现了比自己更先进的 Leader，卸任为 Follower
        _Node.setTerm(other_term);
        _Node.setRole(RaftNode::NodeRole::Follower);
        _Node.setVotedFor(-1);
        return;
    }

    if (other_success) {
        // TODO
    } else {
        // TODO
    }
}

void Raft::_OnVoteRequest(const RequestVoteRequest & _Request, RequestVoteResponse & _Response)
{
    // 获取请求信息
    TermId candidate_term = _Request.term();
    NodeId candidate_id = _Request.candidate_id();
    LogIndex candidate_last_log_index = _Request.last_log_index();
    LogIndex candidate_last_log_term = _Request.last_log_term();

    std::lock_guard<std::mutex> lock(_Mutex);

    if (candidate_term < _Node.getTerm()) {
        // 任期小于自己，拒绝投票
        _Response.set_term(_Node.getTerm());
        _Response.set_voted(false);
        return;
    }

    if (candidate_term > _Node.getTerm()) {
        // 任期大于自己，自己成为 Follower
        _Node.setRole(RaftNode::NodeRole::Follower);
        _Node.setTerm(candidate_term);
        _Node.setVotedFor(-1);
    }

    // 检查是否投过票，当自己为 Candidate 且投过票时也会进入该情况
    NodeId voted = _Node.getVotedFor();
    if (voted != -1 && voted != candidate_id) {
        // 已经投票给了其他人，拒绝投票
        _Response.set_term(_Node.getTerm());
        _Response.set_voted(false);
        return;
    }

    // 判断日志是否比自己更新
    if (!_IsCandidateLogUpToDate(candidate_last_log_index, candidate_last_log_term)) {
        // 不如自己新，拒绝投票
        _Response.set_term(_Node.getTerm());
        _Response.set_voted(false);
        return;
    }

    // 投票成功
    _Node.setVotedFor(candidate_id);
    _Response.set_term(_Node.getTerm());
    _Response.set_voted(true);

    // 重置心跳，避免触发选举
    _Last_heartbeat_recv = std::chrono::steady_clock::now();
}

void Raft::_OnAppendEntriesRequest(const AppendEntriesRequest & _Request, AppendEntriesResponse & _Response)
{
    // 获取请求信息
    TermId leader_term = _Request.term();
    NodeId leader_id = _Request.leader_id();
    LogIndex leader_prev_log_index = _Request.prev_log_index();
    LogIndex leader_prev_log_term = _Request.prev_log_term();

    std::lock_guard<std::mutex> lock(_Mutex);

    if (leader_term < _Node.getTerm()) {
        // 任期比自己小，拒绝心跳
        _Response.set_term(_Node.getTerm());
        _Response.set_success(false);
        return;
    }

    if (leader_term > _Node.getTerm()) {
        // 任期比自己高，有两种情况，一种是别人宣布胜选，另一种是自己经历了宕机、脑裂等事故
        _Node.setTerm(leader_term);
        _Node.setRole(RaftNode::NodeRole::Follower);
        _Node.setVotedFor(-1);
    }

    // 心跳合法，更新时间
    _Last_heartbeat_recv = std::chrono::steady_clock::now();

    // 检查日志是否匹配，是否存在 leader_prev_log_index 处任期为 leader_prev_log_term 的日志
    if (!_Node.match(leader_prev_log_index, leader_prev_log_term)) {
        // 日志不匹配，拒绝心跳
        _Response.set_term(_Node.getTerm());
        _Response.set_success(false);
        return;
    }

    // 开始检测 logentry
    const auto & entries = _Request.entry();
    if (entries.empty()) {
        // 日志为空，是普通心跳报文
        _Response.set_term(_Node.getTerm());
        _Response.set_success(true);
    }

    // 是日志追加请求报文
    // TODO
}

bool Raft::_IsCandidateLogUpToDate(LogIndex _Last_index, TermId _Last_term)
{
    LogIndex my_index = _Node.getLastLogIndex();
    TermId my_term = _Node.getLastLogTerm();

    if (_Last_term != my_term) {
        // 任期不同，比较任期
        return _Last_term > my_term;
    } else {
        // 任期相同，比较索引
        return _Last_index >= my_index;
    }
}

} // namespace WW
