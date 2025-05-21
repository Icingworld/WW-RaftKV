#include "Raft.h"

#include <RaftClient.h>

namespace WW
{

void Raft::run()
{
    _Running.store(true);
    _Thread = std::thread(&Raft::_WorkingThread, this);
}

void Raft::stop()
{
    _Running.store(false);
    if (_Thread.joinable()) {
        _Thread.join();
    }
}

void Raft::_WorkingThread()
{
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
    std::lock_guard<std::mutex> lock(_Mutex);

    // 获取响应信息
    TermId other_term = _Response.term();
    bool other_success = _Response.success();

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

} // namespace WW
