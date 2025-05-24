#include "Raft.h"

#include <random>
#include <algorithm>

#include <RaftClient.h>

namespace WW
{

Raft::Raft(NodeId _Id, const std::vector<RaftPeer> _Peers)
    : _Node(_Id)
    , _Peers(_Peers)
    , _Election_timeout_min(150)
    , _Election_timeout_max(300)
    , _Election_timeout(0)
    , _Heartbeat_timeout(50)
    , _Election_interval(0)
    , _Heartbeat_interval(0)
    , _Vote_count(0)
    , _Out_messages()
{

}

void Raft::tick(int _Delta_time)
{
    _Election_interval += _Delta_time;
    _Heartbeat_interval += _Delta_time;

    if (_Node.isFollower() || _Node.isCandidate()) {
        _TickElection();
    }

    if (_Node.isLeader()) {
        _TickHeartbeat();
    }
}

void Raft::step(const RaftMessage & _Message)
{
    switch (_Message.type) {
        case RaftMessage::MessageType::HeartbeatRequest:
            _HandleHeartbeatRequest(_Message);
            break;
        case RaftMessage::MessageType::HeartbeatResponse:
            _HandleHeartbeatResponse(_Message);
            break;
        case RaftMessage::MessageType::RequestVoteRequest:
            _HandleRequestVoteRequest(_Message);
            break;
        case RaftMessage::MessageType::RequestVoteResponse:
            _HandleRequestVoteResponse(_Message);
            break;
        case RaftMessage::MessageType::AppendEntriesRequest:
            _HandleAppendEntriesRequest(_Message);
            break;
        case RaftMessage::MessageType::AppendEntriesResponse:
            _HandleAppendEntriesResponse(_Message);
            break;
        default:
            break;
    }
}

const std::vector<RaftMessage> & Raft::readReady() const
{
    return _Out_messages;
}

void Raft::clearReady()
{
    _Out_messages.clear();
}

void Raft::_TickElection()
{
    if (_Election_interval >= _Election_timeout) {
        // 超时，Leader 失联，发起选举
        _ResetElectionTimeout();
        _StartElection();
    }
}

void Raft::_TickHeartbeat()
{
    if (_Heartbeat_interval >= _Heartbeat_timeout) {
        // 超时，发送心跳
        _ResetHeartbeatTimeout();
        _SendHeartbeat();
    }
}

void Raft::_StartElection()
{
    // 1. 更新自己的状态
    _Node.switchToCandidate();
    _Node.setVotedFor(_Node.getId());
    _Node.setTerm(_Node.getTerm() + 1);
    _Vote_count = 1;

    // 2. 构造投票请求消息
    for (RaftPeer & peer : _Peers) {
        RaftMessage vote_request;

        vote_request.type = RaftMessage::MessageType::RequestVoteRequest;
        vote_request.from = _Node.getId();
        vote_request.to = peer.getId();
        vote_request.term = _Node.getTerm();
        vote_request.index = _Node.getLastIndex();
        vote_request.log_term = _Node.getLastTerm();

        _Out_messages.emplace_back(vote_request);
    }
}

void Raft::_SendHeartbeat()
{
    // 1. 构造心跳消息
    for (RaftPeer & peer : _Peers) {
        RaftMessage heartbeat_request;

        heartbeat_request.type = RaftMessage::MessageType::HeartbeatRequest;
        heartbeat_request.from = _Node.getId();
        heartbeat_request.to = peer.getId();
        heartbeat_request.commit = _Node.getLastCommitIndex();
        heartbeat_request.entries.clear();

        // 找到该 peer 所持有的最新日志索引
        LogIndex prev_index = peer.getNextIndex() - 1;
        // 从日志中找到该索引对应的任期
        TermId prev_term = _Node.getTerm(prev_index);

        heartbeat_request.index = prev_index;
        heartbeat_request.log_term = prev_term;

        _Out_messages.emplace_back(heartbeat_request);
    }
}

void Raft::_ResetElectionTimeout()
{
    _Election_interval = 0;
}

void Raft::_ResetHeartbeatTimeout()
{
    _Heartbeat_interval = 0;
}

void Raft::_HandleHeartbeatRequest(const RaftMessage & _Message)
{
    // 构造并设置响应消息
    RaftMessage response;
    response.type = RaftMessage::MessageType::RequestVoteResponse;
    response.from = _Message.to;
    response.to = _Node.getId();
    response.term = _Node.getTerm();
    response.reject = true;

    // 1. 比较两节点的任期
    if (_Message.term < response.term) {
        // 任期不如自己，直接拒绝
        _Out_messages.emplace_back(response);
        return;
    }

    if (_Message.term > response.term) {
        // 任期高于自己，修改状态
        _Node.switchToFollower();
        _Node.setTerm(_Message.term);
        _Node.setVotedFor(-1);
        response.term = _Message.term;
    }

    // 2. 重置计时器
    _ResetElectionTimeout();

    // 3. 检查日志是否匹配
    if (!_Node.match(_Message.index, _Message.log_term)) {
        // 日志不匹配
        response.index = _Node.getLastIndex();
        _Out_messages.emplace_back(response);
        return;
    }

    // 4. 更新 commit
    if (_Message.commit > _Node.getLastCommitIndex()) {
        LogIndex last_index = _Node.getLastIndex();
        _Node.setLastCommitIndex(std::min(_Message.commit, last_index));
    }

    // 5. 响应成功
    response.reject = false;
    response.index = _Node.getLastIndex();

    _Out_messages.emplace_back(response);
}

void Raft::_HandleRequestVoteRequest(const RaftMessage & _Message)
{
    // 构造并设置响应消息
    RaftMessage response;
    response.type = RaftMessage::MessageType::RequestVoteResponse;
    response.from = _Message.to;
    response.to = _Node.getId();
    response.term = _Node.getTerm();
    response.reject = true;

    // 1. 比较两节点的任期
    if (_Message.term < response.term) {
        // 任期不如自己，直接拒绝
        _Out_messages.emplace_back(response);
        return;
    }

    if (_Message.term > response.term) {
        // 任期大于自己，自己转换为 Follower
        _Node.switchToFollower();
        _Node.setTerm(_Message.term);
        _Node.setVotedFor(-1);
        // 更新响应
        response.term = _Message.term;
    }

    // 2. 检查自己是否已经投过票
    if (_Node.getVotedFor() != -1 && _Node.getVotedFor() != response.to) {
        // 已经投过票且不是该节点，拒绝投票
        _Out_messages.emplace_back(response);
        return;
    }

    // 3. 检查日志是否匹配
    if (!_LogUpToDate(_Message.index, _Message.log_term)) {
        // 日志不匹配，拒绝投票
        _Out_messages.emplace_back(response);
        return;
    }

    // 4. 条件通过，投票
    _Node.setVotedFor(response.to);
    response.reject = false;

    _Out_messages.emplace_back(response);
}

void Raft::_HandleHeartbeatResponse(const RaftMessage & _Message)
{
    if (!_Node.isLeader()) {
        // 已经不是 Leader
        return;
    }

    // 读取响应消息
    NodeId other_id = _Message.from;
    TermId other_term = _Message.term;
    LogIndex other_index = _Message.index;
    bool other_reject = _Message.reject;

    // 1. 检查任期
    if (other_term > _Node.getTerm()) {
        // 对方任期更高，退位为 Follower
        _Node.switchToFollower();
        _Node.setTerm(other_term);
        _Node.setVotedFor(-1);
        return;
    }

    // 2. 找到对应的 peer
    auto it = _Peers.begin();
    for (; it != _Peers.end(); ++it) {
        if (it->getId() == other_id) {
            break;
        }
    }

    if (it == _Peers.end()) {
        // 没找到这个节点
        return;
    }

    RaftPeer & peer = *it;

    // 3. 判断对方日志是否匹配，通过判断是否拒绝
    if (other_reject) {
        // 被拒绝了，说明对方日志不匹配，回退一个索引，等下一次发送心跳时重试
        LogIndex next_index = peer.getNextIndex();
        if (next_index > 1) {
            peer.setNextIndex(next_index - 1);
        }
        return;
    }

    // 4. 开始推进日志，由于心跳包是空的，索引就是起始位置
    peer.setMatchIndex(other_index);
    peer.setNextIndex(other_index + 1);

    // 5. 统计所有节点的 matchIndex
    std::vector<LogIndex> match_indexes;
    match_indexes.emplace_back(_Node.getLastIndex());

    for (const RaftPeer & peer : _Peers) {
        match_indexes.emplace_back(peer.getMatchIndex());
    }

    // TODO 有优化空间
    std::sort(match_indexes.begin(), match_indexes.end());
    LogIndex majority_match = match_indexes[match_indexes.size() / 2];

    // 6. 如果该日志是当前任期的，同步
    if (_Node.getTerm(majority_match) == _Node.getTerm()) {
        _Node.setLastCommitIndex(majority_match);
    }
}

void Raft::_HandleRequestVoteResponse(const RaftMessage & _Message)
{
    if (!_Node.isCandidate()) {
        // 已经不是 Candidate，退选或胜选
        return;
    }

    // 读取响应消息
    TermId other_term = _Message.term;
    bool other_reject = _Message.reject;

    // 1. 检查任期
    if (other_term > _Node.getTerm()) {
        // 任期大于自己，退出选举
        _Node.switchToFollower();
        _Node.setTerm(other_term);
        _Node.setVotedFor(-1);
        return;
    }

    // 2. 判断是否投票
    if (!other_reject) {
        // 增加一票
        ++_Vote_count;

        // 判断是否已经胜选
        if (_Vote_count > (_Peers.size() / 2)) {
            // 超过半数，胜选
            _Node.switchToLeader();

            // Leader 需要初始化 peer 的两个索引
            for (RaftPeer & peer : _Peers) {
                peer.setMatchIndex(0);
                peer.setNextIndex(_Node.getLastIndex() + 1);
            }

            // 立即发送心跳宣布自己成为 Leader
            _SendHeartbeat();
        }
    }
}

void Raft::_HandleAppendEntriesRequest(const RaftMessage & _Message)
{

}

void Raft::_HandleAppendEntriesResponse(const RaftMessage & _Message)
{

}

int Raft::_GetRandomTimeout(int _Timeout_min, int _Timeout_max) const
{
    // 创建随机数引擎
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(_Timeout_min, _Timeout_max);

    return dist(rng);
}

bool Raft::_LogUpToDate(LogIndex _Last_index, TermId _Last_term)
{
    LogIndex my_index = _Node.getLastIndex();
    TermId my_term = _Node.getLastTerm();

    if (_Last_term != my_term) {
        // 任期不同，比较任期
        return _Last_term > my_term;
    } else {
        // 任期相同，比较索引
        return _Last_index >= my_index;
    }
}

} // namespace WW
