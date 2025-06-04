#include "Raft.h"

#include <random>
#include <algorithm>
#include <fstream>

#include <RaftLogger.h>
#include <RaftPersist.pb.h>

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
    , _Is_snapshoting(false)
    , _Is_dirty(false)
    , _Inner_messages()
    , _Outter_messages()
{
    _Election_timeout = _GetRandomTimeout(_Election_timeout_min, _Election_timeout_max);
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
        case RaftMessage::MessageType::ApplySnapShot:
            _ApplySnapShot(_Message);
            break;
        case RaftMessage::MessageType::OperationRequest:
            _HandleOperationRequest(_Message);
            break;
        default:
            break;
    }
}

const std::vector<RaftMessage> & Raft::readInnerMessage() const
{
    return _Inner_messages;
}

const RaftMessage & Raft::readOutterMessage() const
{
    return _Outter_messages;
}

void Raft::clearInnerMessage()
{
    _Inner_messages.clear();
}

NodeId Raft::getId() const
{
    return _Node.getId();
}

void Raft::_TickElection()
{
    if (_Election_interval >= _Election_timeout) {
        // 超时，Leader 失联，发起选举
        DEBUG("leader missing, start election");
        _ResetElectionTimeout();
        _StartElection();
    }
}

void Raft::_TickHeartbeat()
{
    if (_Heartbeat_interval >= _Heartbeat_timeout) {
        // 超时，发送心跳
        // DEBUG("send heartbeat");
        _ResetHeartbeatTimeout();
        _SendAppendEntries(true);
    }
}

void Raft::_StartElection()
{
    // 1. 更新自己的状态
    _Node.switchToCandidate();
    _Node.setVotedFor(_Node.getId());
    _Node.setTerm(_Node.getTerm() + 1);
    _Vote_count = 1;

    // 持久化
    _Is_dirty = true;
    _Persist();

    // 投票请求所需的数据
    NodeId this_id = _Node.getId();
    TermId this_term = _Node.getTerm();
    LogIndex this_last_log_index = _Node.getLastIndex();
    LogIndex this_last_log_term = _Node.getLastTerm();

    DEBUG("current term: %zu", this_term);

    // 2. 发送投票请求消息
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == this_id) {
            continue;
        }

        // 构造投票请求上下文
        RaftMessage vote_request;
        vote_request.type = RaftMessage::MessageType::RequestVoteRequest;
        vote_request.from = this_id;
        vote_request.to = peer.getId();
        vote_request.term = this_term;
        vote_request.index = this_last_log_index;
        vote_request.log_term = this_last_log_term;

        _Inner_messages.emplace_back(vote_request);
    }

    // 3. 重置超时时间，防止再次发起选举
    _ResetElectionTimeout();
}

void Raft::_SendAppendEntries(bool _IsHearbeat)
{
    // 日志同步请求所需要的数据
    NodeId this_id = _Node.getId();
    TermId this_term = _Node.getTerm();
    LogIndex this_commit = _Node.getLastCommitIndex();

    // 准备发送日志同步消息
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == _Node.getId()) {
            continue;
        }

        // 构造日志同步消息
        RaftMessage heartbeat_request;
        heartbeat_request.type = RaftMessage::MessageType::AppendEntriesRequest;
        heartbeat_request.from = this_id;
        heartbeat_request.to = peer.getId();
        heartbeat_request.term = this_term;
        heartbeat_request.commit = this_commit;
        
        if (!_IsHearbeat) {
            // 添加日志条目
            // 从该 Follower 的 nextIndex 开始添加日志
            heartbeat_request.entries = _Node.getLogFrom(peer.getNextIndex());
        }

        // 找到该 peer 所持有的最新日志索引
        LogIndex prev_index = peer.getNextIndex() - 1;
        // 从日志中找到该索引对应的任期
        // DEBUG("prev_index:%d, base_index:%d, last_index:%d", prev_index, _Node.getBaseIndex(), _Node.getLastIndex());
        TermId prev_term = _Node.getTerm(prev_index);

        heartbeat_request.index = prev_index;
        heartbeat_request.log_term = prev_term;

        _Inner_messages.emplace_back(heartbeat_request);
    }

    // 重置发送时间，防止发送过快
    _ResetHeartbeatTimeout();
}

void Raft::_ResetElectionTimeout()
{
    _Election_interval = 0;

    // 重新生成超时时间
    _Election_timeout = _GetRandomTimeout(_Election_timeout_min, _Election_timeout_max);
}

void Raft::_ResetHeartbeatTimeout()
{
    _Heartbeat_interval = 0;
}

void Raft::_HandleRequestVoteRequest(const RaftMessage & _Message)
{
    // 获取请求上下文中的信息
    NodeId other_id = _Message.from;
    TermId other_term = _Message.term;
    LogIndex other_last_log_index = _Message.index;
    TermId other_last_log_term = _Message.log_term;

    // 构造并设置响应上下文消息
    RaftMessage response;
    response.type = RaftMessage::MessageType::RequestVoteResponse;
    response.from = _Node.getId();
    response.to = other_id;
    response.term = _Node.getTerm();
    response.reject = true;

    DEBUG("receive vote request from node: %d", response.to);

    // 1. 比较两节点的任期
    if (other_term < response.term) {
        // 任期不如自己，直接拒绝
        DEBUG("term: %zu less than self: %zu, refuse to vote", other_term, response.term);
        _Outter_messages = response;
        return;
    }

    if (other_term > response.term) {
        // 任期大于自己，自己转换为 Follower
        DEBUG("term: %zu larger than self: %zu, switch to Follower", other_term, response.term);
        _Node.switchToFollower();
        _ResetElectionTimeout();
        _Node.setTerm(other_term);
        _Node.setVotedFor(-1);
        // 更新响应
        response.term = other_term;

        _Is_dirty = true;
    }

    // 2. 检查自己是否已经投过票
    if (_Node.getVotedFor() != -1 && _Node.getVotedFor() != response.to) {
        // 已经投过票且不是该节点，拒绝投票
        DEBUG("already voted for node: %d, refuse to vote", _Node.getVotedFor());
        _Outter_messages = response;

        // 这里不需要持久化，如果是刚刚退选的，则一定可以投票，如果早就是 Follower，则之前就持久化过了
        return;
    }

    // 3. 检查日志是否匹配
    if (!_LogUpToDate(other_last_log_index, other_last_log_term)) {
        // 日志不匹配，拒绝投票
        DEBUG("log (index:%d , term:%zu) doesn't match (index:%d, term:%zu), refuse to vote", 
                other_last_log_index, other_last_log_term, _Node.getLastIndex(), _Node.getLastTerm());
        _Outter_messages = response;
        return;
    }

    // 4. 条件通过，投票
    DEBUG("approve, vote for it");
    _Node.setVotedFor(response.to);
    response.reject = false;

    // 持久化
    _Is_dirty = true;
    _Persist();

    _Outter_messages = response;
}

void Raft::_HandleRequestVoteResponse(const RaftMessage & _Message)
{
    if (!_Node.isCandidate()) {
        // 不是 Candidate，已经退选或胜选
        return;
    }

    // 获取响应上下文消息中的信息
    TermId other_term = _Message.term;
    bool other_reject = _Message.reject;

    // 1. 检查任期
    if (other_term > _Node.getTerm()) {
        // 任期大于自己，退出选举
        DEBUG("withdraw the election");
        _Node.switchToFollower();
        _ResetElectionTimeout();
        _Node.setTerm(other_term);
        _Node.setVotedFor(-1);

        // 持久化
        _Is_dirty = true;
        _Persist();

        return;
    }

    // 2. 判断是否投票
    if (!other_reject) {
        // 增加一票
        ++_Vote_count;

        // 判断是否已经胜选
        if (_Vote_count > (_Peers.size() / 2)) {
            // 超过半数，胜选
            DEBUG("win the election, switch to leader");
            _Node.switchToLeader();
            _Node.setVotedFor(-1);

            // 持久化
            _Is_dirty = true;
            _Persist();

            // Leader 需要初始化 peer 的两个索引
            for (RaftPeer & peer : _Peers) {
                if (peer.getId() == _Node.getId()) {
                    continue;
                }

                peer.setMatchIndex(0);
                peer.setNextIndex(_Node.getLastIndex() + 1);
            }

            // 立即发送心跳宣布自己成为 Leader
            _ResetHeartbeatTimeout();
            _SendAppendEntries(true);
        }
    }
}

void Raft::_HandleAppendEntriesRequest(const RaftMessage & _Message)
{
    // 获取日志同步请求上下文消息中的信息
    NodeId other_id = _Message.from;
    TermId other_term = _Message.term;
    LogIndex other_last_log_index = _Message.index;
    TermId other_last_log_term = _Message.log_term;
    const std::vector<WW::RaftLogEntry> & other_entries = _Message.entries;
    LogIndex other_commit = _Message.commit;

    // 构造并设置响应消息
    RaftMessage response;
    response.type = RaftMessage::MessageType::AppendEntriesResponse;
    response.from = _Node.getId();
    response.to = other_id;
    response.term = _Node.getTerm();
    response.reject = true;

    // 1. 比较两节点的任期
    if (other_term < response.term) {
        // 任期不如自己，直接拒绝
        DEBUG("term: %zu less than self: %zu, refuse append entries", other_term, response.term);
        _Outter_messages = response;
        return;
    }

    if (other_term > response.term) {
        // 任期高于自己，修改状态
        DEBUG("term: %zu larger than self: %zu, switch to Follower", other_term, response.term);
        _Node.switchToFollower();
        _ResetElectionTimeout();
        _Node.setTerm(other_term);
        _Node.setVotedFor(-1);
        response.term = other_term;

        _Is_dirty = true;
    }

    // 2. 检查日志是否匹配
    if (!_Node.match(other_last_log_index, other_last_log_term)) {
        // 日志存在冲突，需要 Leader 进行回退
        DEBUG("log (index:%d , term:%zu) doesn't match (index:%d, term:%zu), refuse append entries",
                other_last_log_index, other_last_log_term, _Node.getLastIndex(), _Node.getLastTerm());
        // 截断该索引之后的所有日志
        _Node.truncateAfter(other_last_log_index);
        // 告知自己的索引位置，冲突情况下不使用
        response.index = _Node.getLastIndex();
        _Outter_messages = response;

        // 持久化
        _Is_dirty = true;
        _Persist();

        return;
    }

    // 到这里已经收到合法报文，重置选举超时时间
    _ResetElectionTimeout();

    // 3. 更新 commit
    if (other_commit > _Node.getLastCommitIndex()) {
        LogIndex last_index = _Node.getLastIndex();
        _Node.setLastCommitIndex(std::min(other_commit, last_index));
    }

    // 4. 同步日志
    if (!other_entries.empty()) {
        // 不是心跳，需要同步日志
        for (const RaftLogEntry & entry : other_entries) {
            DEBUG("append entry:");
            DEBUG("term: %zu, command: %s", entry.getTerm(), entry.getCommand().c_str());
            _Node.append(entry);
        }

        _Is_dirty = true;
    }

    // 应用日志
    _ApplyCommitedLogs();

    // 检测是否需要压缩快照
    _CheckIfNeedSnapShot(_Node.getLastCommitIndex());

    // 5. 响应成功
    // DEBUG("append entries success");       // too noisy

    // 设置 Leader ID
    _Node.setLeaderId(other_id);

    response.index = _Node.getLastIndex();
    response.reject = false;

    _Outter_messages = response;

    // 持久化
    _Persist();
}

void Raft::_HandleAppendEntriesResponse(const RaftMessage & _Message)
{
    if (!_Node.isLeader()) {
        // 已经不是 Leader
        return;
    }

    // 读取响应上下文消息中的信息
    NodeId other_id = _Message.from;
    TermId other_term = _Message.term;
    LogIndex other_log_index = _Message.index;
    bool other_reject = _Message.reject;

    // 1. 检查任期
    if (other_term > _Node.getTerm()) {
        // 对方任期更高，退位为 Follower
        _Node.switchToFollower();
        _ResetElectionTimeout();
        _Node.setTerm(other_term);
        _Node.setVotedFor(-1);

        // 持久化
        _Is_dirty = true;
        _Persist();
        return;
    }

    // 找到对应的 peer
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

    RaftPeer & other_node = *it;

    // 2. 判断是否同步
    if (other_reject) {
        // 对方节点拒绝了，说明日志存在不同步，下一次从前一条日志开始同步
        LogIndex next_index = other_node.getNextIndex();
        if (next_index > 1) {
            other_node.setNextIndex(next_index - 1);
        }
        return;
    }

    // 3. 开始推进日志，由于心跳包是空的，索引就是起始位置
    other_node.setMatchIndex(other_log_index);
    other_node.setNextIndex(other_log_index + 1);

    // 4. 统计所有节点的 matchIndex
    std::vector<LogIndex> match_indexes;
    match_indexes.emplace_back(_Node.getLastIndex());

    for (const RaftPeer & peer : _Peers) {
        match_indexes.emplace_back(peer.getMatchIndex());
    }

    std::sort(match_indexes.begin(), match_indexes.end());
    LogIndex majority_match = match_indexes[match_indexes.size() / 2];

    // 5. 如果该日志是当前任期的，同步
    if (_Node.getTerm(majority_match) == _Node.getTerm()) {
        _Node.setLastCommitIndex(majority_match);

        // 应用日志
        _ApplyCommitedLogs();

        // 检查是否需要压缩成快照
        _CheckIfNeedSnapShot(_Node.getLastCommitIndex());
    }
}

void Raft::_HandleOperationRequest(const RaftMessage & _Message)
{
    // 构造响应上下文消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::OPerationResponse;
    message.reject = true;

    if (!_Node.isLeader()) {
        // 不是 Leader，返回 Leader 地址
        DEBUG("not leader, refuse operation");
        message.to = _Node.getLeaderId();
        _Outter_messages = message;
        return;
    }

    // 同意操作
    message.reject = false;
    message.from = _Node.getId();

    // 生成并添加一条日志
    if (_Message.op_type != RaftMessage::OperationType::GET) {
        DEBUG("append a new log entry, term: %zu, command: %s", _Node.getTerm(), _Message.command.c_str());
        RaftLogEntry new_log(_Node.getTerm(), _Message.command);
        _Node.append(new_log);

        // 发送日志同步请求
        _SendAppendEntries(false);

        // 持久化
        _Is_dirty = true;
        _Persist();
    }

    _Outter_messages = message;
}

void Raft::_ApplySnapShot(const RaftMessage & _Message)
{
    // 读取上下文消息
    NodeId id = _Message.from;
    LogIndex index = _Message.index;
    TermId log_term = _Message.log_term;
    std::string snapshot = _Message.snapshot;

    if (id == _Node.getId()) {
        // 是自己创建快照，说明应用层已经创建/安装快照，直接开始截断
        _Node.truncateBefore(index + 1);

        // 持久化
        _Is_dirty = true;
        _Persist();

        // 确认状态
        _Is_snapshoting = false;
    } else {
        // 是 Leader 发送的快照
        if (index <= _Node.getLastAppliedIndex()) {
            // 已经至少比快照更新了，拒绝安装
            return;
        }

        // 同意应用快照
        RaftMessage message;
        message.type = RaftMessage::MessageType::ApplySnapShot;
        message.from = _Node.getId();
        message.index = index;
        message.log_term = log_term;
        message.command = snapshot;

        _Inner_messages.emplace_back(message);
    }
}

void Raft::_ApplyCommitedLogs()
{
    // 构造一个上下文消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::LogEntriesApply;

    while (_Node.getLastAppliedIndex() < _Node.getLastCommitIndex()) {
        DEBUG("last applied: %d, last commited: %d", _Node.getLastAppliedIndex(), _Node.getLastCommitIndex());
        _Node.setLastAppliedIndex(_Node.getLastAppliedIndex() + 1);

        const RaftLogEntry & entry = _Node.getLog(_Node.getLastAppliedIndex());
        DEBUG("apply commitd logs");
        message.entries.emplace_back(entry);
    }

    // 传出上下文
    _Inner_messages.emplace_back(message);
}

void Raft::_TakeSnapShot()
{
    // 构造一个上下文消息
    // 这里只是决定要创建快照
    RaftMessage message;
    message.type = RaftMessage::MessageType::TakeSnapShot;
    message.index = _Node.getLastAppliedIndex();
    message.log_term = _Node.getTerm(message.index);

    // 传出上下文
    _Inner_messages.emplace_back(message);
}

void Raft::_CheckIfNeedSnapShot(LogIndex _Index)
{
    if (_Is_snapshoting) {
        return;
    }

    // DEBUG("check snapshot index:%d", _Index);

    _Is_snapshoting = true;

    // 设置日志阈值为 1000 条
    // constexpr int MAX_LOG_SIZE = 1000;
    constexpr int MAX_LOG_SIZE = 3;     // for test

    LogIndex snapshot_index = _Node.getBaseIndex();

    if (_Index > snapshot_index && _Index - snapshot_index >= MAX_LOG_SIZE) {
        // 超出阈值，准备创建快照
        _TakeSnapShot();
        return;
    }

    _Is_snapshoting = false;
}

int Raft::_GetRandomTimeout(int _Timeout_min, int _Timeout_max) const
{
    // 创建全局随机数引擎
    static std::mt19937 rng(std::random_device{}());
    // 分布需要每次都创建
    std::uniform_int_distribution<int> dist(_Timeout_min, _Timeout_max);

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

void Raft::_Persist()
{
    if (!_Is_dirty) {
        return;
    }

    // 创建一个结构体
    PersistData persist_data;

    // 添加基础信息
    persist_data.set_term(_Node.getTerm());
    persist_data.set_voted_for(_Node.getVotedFor());

    // 拷贝剩余日志条目
    const std::vector<RaftLogEntry> & log_entries = _Node.getLogFrom(_Node.getBaseIndex());
    for (const RaftLogEntry & entry : log_entries) {
        PersistLogEntry * ptr = persist_data.add_entries();
        ptr->set_term(entry.getTerm());
        ptr->set_command(entry.getCommand());
    }

    // 添加快照元信息
    persist_data.set_snapshot_index(_Node.getSnapShotIndex());
    persist_data.set_snapshot_term(_Node.getSnapShotTerm());

    // 序列化
    std::string persist_str;
    if (!persist_data.SerializeToString(&persist_str)) {
        ERROR("persist data serialization failed");
        return;
    }

    // 写入文件
    std::string file_path = "raftnode_" + std::to_string(_Node.getId()) + ".data";
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        ERROR("open raftnode data file failed");
        return;
    }
    out.write(persist_str.data(), persist_str.size());
    out.close();

    DEBUG("raft data persisted");

    // 重置标记
    _Is_dirty = false;
}

bool Raft::load()
{
    // 读取持久化文件
    std::string file_path = "raftnode_" + std::to_string(_Node.getId()) + ".data";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        ERROR("open raftnode data file failed");
        return false;
    }
    
    // 反序列化
    PersistData persist_data;
    if (!persist_data.ParseFromIstream(&in)) {
        ERROR("parse persisted data failed");
        return false;
    }

    in.close();

    // 加载持久化信息
    _Node.setTerm(persist_data.term());
    _Node.setVotedFor(persist_data.voted_for());
    _Node.setSnapShotIndex(persist_data.snapshot_index());
    _Node.setSnapShotTerm(persist_data.snapshot_term());

    // 加载未快照日志
    for (const PersistLogEntry & persist_entry : persist_data.entries()) {
        RaftLogEntry entry(persist_entry.term(), persist_entry.command());
        _Node.append(entry);
    }

    DEBUG("raft persisted data loaded");
    return true;
}

} // namespace WW
