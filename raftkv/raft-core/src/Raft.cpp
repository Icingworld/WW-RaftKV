#include "Raft.h"

#include <random>
#include <algorithm>
#include <fstream>

#include <ConsoleSink.h>
#include <RaftPersist.pb.h>

namespace WW
{

Raft::Raft(NodeId _Id, const std::vector<RaftPeer> _Peers)
    : _Id(_Id)
    , _Role(RaftRole::Follower)
    , _Term(0)
    , _Vote_count(0)
    , _Voted_for(-1)
    , _Leader_id(-1)
    , _Logs()
    , _Base_index(1)
    , _Last_log_index(0)
    , _Last_log_term(0)
    , _Last_included_index(0)
    , _Last_included_term(0)
    , _Last_commit_index(0)
    , _Last_applied_index(0)
    , _Is_applying(false)
    , _Is_snapshoting(false)
    , _Peers(_Peers)
    , _Inner_channel()
    , _Outter_channel()
    , _Election_timeout_min(150)
    , _Election_timeout_max(300)
    , _Heartbeat_timeout(50)
    , _Election_deadline()
    , _Heartbeat_deadline()
    , _Running(false)
    , _Raft_thread()
    , _Message_thread()
    , _Logger(Logger::getSyncLogger("Raft"))
{
    // 设置日志参数
    _Logger.setLevel(LogLevel::Debug);
    std::shared_ptr<ConsoleSink> console_sink = std::make_shared<ConsoleSink>();
    _Logger.addSink(console_sink);

    // 初始为 Follower，只需要重置选举超时时间
    _ResetElectionDeadline();
}

Raft::~Raft()
{
    if (_Running.load()) {
        stop();
    }
}

void Raft::start()
{
    _Running.store(true);

    // 启动 Raft 定时线程
    _Raft_thread = std::thread(&Raft::_RaftLoop, this);
}

void Raft::startMessage()
{
    _Running.store(true);

    // 启动消息队列线程
    _Message_thread = std::thread(&Raft::_GetOutterMessage, this);
}

void Raft::stop()
{
    _Running.store(false);

    // 关闭消息队列线程
    if (_Message_thread.joinable()) {
        _Message_thread.join();
    }

    // 关闭 Raft 定时线程
    if (_Raft_thread.joinable()) {
        _Raft_thread.join();
    }
}

void Raft::_RaftLoop()
{
    while (_Running.load()) {
        // 计算下一次超时时间
        std::chrono::steady_clock::time_point next_timeout;
        if (_Role == RaftRole::Leader) {
            next_timeout = _Heartbeat_deadline;
        } else {
            next_timeout = _Election_deadline;
        }

        // 判断是否超时
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (now >= _Election_deadline && _Role != RaftRole::Leader) {
            _Logger.debug("election timeout, switch to candidate and start election");
            _BecomeCandidate();
        }
        if (now >= _Heartbeat_deadline && _Role == RaftRole::Leader) {
            _SendAppendEntries(true);
        }
    }
}

void Raft::_GetOutterMessage()
{
    while (_Running.load()) {
        std::unique_ptr<RaftMessage> message = std::move(_Outter_channel.pop(-1));
        if (message != nullptr) {
            _HandleMessage(std::move(message));
        }
    }
}

std::unique_ptr<RaftMessage> Raft::readReady(int _Wait_ms)
{
    return std::move(_Inner_channel.pop(_Wait_ms));
}

NodeId Raft::getId() const
{
    return _Id;
}

void Raft::_HandleMessage(std::unique_ptr<RaftMessage> _Message)
{
    switch (_Message->type) {
    case RaftMessage::MessageType::RequestVoteRequest: {
        const RaftRequestVoteRequestMessage * request_vote_request_message = static_cast<const RaftRequestVoteRequestMessage *>(_Message.get());
        _HandleRequestVoteRequest(request_vote_request_message);
        break;
    }
    case RaftMessage::MessageType::RequestVoteResponse: {
        const RaftRequestVoteResponseMessage * request_vote_response_message = static_cast<const RaftRequestVoteResponseMessage *>(_Message.get());
        _HandleRequestVoteResponse(request_vote_response_message);
        break;
    }
    case RaftMessage::MessageType::AppendEntriesRequest: {
        const RaftAppendEntriesRequestMessage * append_entries_request_message = static_cast<const RaftAppendEntriesRequestMessage *>(_Message.get());
        _HandleAppendEntriesRequest(append_entries_request_message);
        break;
    }
    case RaftMessage::MessageType::AppendEntriesResponse: {
        const RaftAppendEntriesResponseMessage * append_entries_response_message = static_cast<const RaftAppendEntriesResponseMessage *>(_Message.get());
        _HandleAppendEntriesResponse(append_entries_response_message);
        break;
    }
    case RaftMessage::MessageType::InstallSnapshotRequest: {
        const RaftInstallSnapshotRequestMessage * install_snapshot_request_message = static_cast<const RaftInstallSnapshotRequestMessage *>(_Message.get());
        _HandleInstallSnapshotRequest(install_snapshot_request_message);
        break;
    }
    case RaftMessage::MessageType::InstallSnapshotResponse: {
        const RaftInstallSnapshotResponseMessage * install_snapshot_response_message = static_cast<const RaftInstallSnapshotResponseMessage *>(_Message.get());
        _HandleInstallSnapshotResponse(install_snapshot_response_message);
        break;
    }
    case RaftMessage::MessageType::KVOperationRequest: {
        const KVOperationRequestMessage * kv_operation_request_message = static_cast<const KVOperationRequestMessage *>(_Message.get());
        _HandleKVOperationRequest(kv_operation_request_message);
        break;
    }
    case RaftMessage::MessageType::ApplyCommitLogsResponse: {
        const ApplyCommitLogsResponseMessage * apply_commit_logs_response_message = static_cast<const ApplyCommitLogsResponseMessage *>(_Message.get());
        _HandleApplyCommitLogs(apply_commit_logs_response_message);
        break;
    }
    case RaftMessage::MessageType::GenerateSnapshotResponse: {
        const GenerateSnapshotResponseMessage * generate_snapshot_response_message = static_cast<const GenerateSnapshotResponseMessage *>(_Message.get());
        _HandleGenerateSnapshot(generate_snapshot_response_message);
        break;
    }
    case RaftMessage::MessageType::ApplySnapshotResponse: {
        const ApplySnapshotResponseMessage * apply_snapshot_response_message = static_cast<const ApplySnapshotResponseMessage *>(_Message.get());
        _HandleApplySnapshot(apply_snapshot_response_message);
        break;
    }
    default:
        break;
    }
}

void Raft::_BecomeFollower(NodeId _Leader_id, TermId _Leader_term)
{
    _Role = RaftRole::Follower;
    _Term = _Leader_term;
    this->_Leader_id = _Leader_id;
    _Voted_for = -1;

    _ResetElectionDeadline();
    _Persist();
}

void Raft::_BecomeCandidate()
{
    _Role = RaftRole::Candidate;
    ++_Term;
    _Voted_for = _Id;
    _Vote_count = 1;
    _Persist();

    // 发送竞选请求
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == _Id) {
            continue;
        }

        // 构造上下文
        RaftRequestVoteRequestMessage request_vote_request_message;
        request_vote_request_message.from = _Id;
        request_vote_request_message.term = _Term;
        request_vote_request_message.last_log_index = _Last_log_index;
        request_vote_request_message.last_log_term = _Last_log_term;
        request_vote_request_message.to = peer.getId();

        // 发送到应用层
        _Inner_channel.push(std::move(request_vote_request_message));
    }

    // 重置选举超时
    _ResetElectionDeadline();
}

void Raft::_BecomeLeader()
{
    _Role = RaftRole::Leader;
    _Leader_id = _Id;
    _Vote_count = 0;
    _Voted_for = -1;
    _Persist();

    // 初始化所有节点状态
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == _Id) {
            continue;
        }

        peer.setMatchIndex(0);
        peer.setNextIndex(_Last_log_index + 1);
    }

    // 立即发送心跳宣布胜选
    _SendAppendEntries(true);

    // 重置心跳超时
    _ResetHeartbeatDeadline();
}

void Raft::_SendAppendEntries(bool _Is_heartbeat)
{
    // _Logger.debug("this id: " + std::to_string(_Id) + ", term: " + std::to_string(_Term));
    
    for (RaftPeer & peer : _Peers) {
        if (peer.getId() == _Id) {
            continue;
        }

        if (peer.getNextIndex() < _Base_index) {
            // 落后太多，需要发送快照
            _Logger.debug("node: " + std::to_string(peer.getId()) + " too far behind, send install snapshot");
            RaftInstallSnapshotRequestMessage install_snapshot_request_message;
            install_snapshot_request_message.term = _Term;
            install_snapshot_request_message.from = _Id;
            install_snapshot_request_message.to = peer.getId();
            install_snapshot_request_message.last_included_index = _Last_included_index;
            install_snapshot_request_message.last_included_term = _Last_included_term;

            _Inner_channel.push(std::move(install_snapshot_request_message));
        } else {
            // 正常发送心跳/日志同步
            // 构造上下文
            RaftAppendEntriesRequestMessage append_entries_request_message;
            append_entries_request_message.term = _Term;
            append_entries_request_message.from = _Id;
            append_entries_request_message.to = peer.getId();
            append_entries_request_message.leader_commit = _Last_commit_index;
            append_entries_request_message.prev_log_index = peer.getNextIndex() - 1;
            append_entries_request_message.prev_log_term = _GetTermAt(append_entries_request_message.prev_log_index);

            if (!_Is_heartbeat) {
                // 添加日志条目
                append_entries_request_message.entries = _GetLogFrom(peer.getNextIndex());
            }

            _Inner_channel.push(std::move(append_entries_request_message));
        }
    }

    _ResetHeartbeatDeadline();
}

void Raft::_HandleRequestVoteRequest(const RaftRequestVoteRequestMessage * _Message)
{
    // 读取上下文信息
    NodeId other_id = _Message->from;
    TermId other_term = _Message->term;
    LogIndex other_last_log_index = _Message->last_log_index;
    TermId other_last_log_term = _Message->last_log_term;
    SequenceType sequence_id = _Message->seq;

    _Logger.debug("receive vote request from node: " + std::to_string(other_id) + ", term: " + std::to_string(other_term));

    // 构造响应上下文
    RaftRequestVoteResponseMessage request_vote_response_message;
    request_vote_response_message.seq = sequence_id;
    request_vote_response_message.term = _Term;
    request_vote_response_message.vote_granted = false;

    // 1. 比较节点任期
    if (other_term < _Term) {
        // 任期小于自己，拒绝投票
        _Logger.debug("candidate term: " + std::to_string(other_term) + " less than self: " + std::to_string(_Term) + ", refuse to vote");

        _Inner_channel.push(std::move(request_vote_response_message));
        return;
    }

    if (other_term > _Term) {
        // 任期大于自己，更新状态
        _Logger.debug("candidate term: " + std::to_string(other_term) + " larger than self: " + std::to_string(_Term) + ", switch to follower");
        _BecomeFollower(-1, other_term);
        request_vote_response_message.term = _Term;
    }

    // 2.1 判断是否已经投过票，或者已经投给了目标
    if (_Voted_for != -1 && _Voted_for != other_id) {
        // 已经投给了其他节点，拒绝投票
        _Logger.debug("already voted for node: " + std::to_string(_Voted_for) + ", refuse to vote");
        _Inner_channel.push(std::move(request_vote_response_message));
        return;
    }

    // 2.2 判断对方日志是否至少比自己新
    if (!_LogUpToDate(other_last_log_index, other_last_log_term)) {
        // 日志不如自己新，拒绝投票
        _Logger.debug("candidate's log not up-to-date, refuse to vote");
        _Inner_channel.push(std::move(request_vote_response_message));
        return;
    }

    // 3. 同意投票
    _Logger.debug("approve to vote");
    _Voted_for = other_id;
    request_vote_response_message.vote_granted = true;
    _Persist();

    _Inner_channel.push(std::move(request_vote_response_message));
}

void Raft::_HandleRequestVoteResponse(const RaftRequestVoteResponseMessage * _Message)
{
    if (_Role != RaftRole::Candidate) {
        // 不是 Candidate，已经退选或胜选
        return;
    }

    // 读取上下文信息
    TermId other_term = _Message->term;
    bool other_vote_granted = _Message->vote_granted;

    // 1. 比较节点任期
    if (other_term > _Term) {
        // 任期比自己大，退选
        _Logger.debug("candidate term: " + std::to_string(other_term) + " less than self: " + std::to_string(_Term) + ", withdraw from the election");
        _BecomeFollower(-1, other_term);
        return;
    }

    if (other_term < _Term) {
        // 非法响应，忽略
        return;
    }

    // 2. 判断是否投票
    if (other_vote_granted) {
        ++_Vote_count;

        if (_Vote_count > (_Peers.size() / 2)) {
            // 获得超过半数选票，胜选
            _Logger.debug("obtain majority of votes, win the election");
            _BecomeLeader();
        }
    }
}

void Raft::_HandleAppendEntriesRequest(const RaftAppendEntriesRequestMessage * _Message)
{
    // 读取上下文信息
    SequenceType sequence_id = _Message->seq;
    NodeId other_id = _Message->from;
    TermId other_term = _Message->term;
    LogIndex prev_log_index = _Message->prev_log_index;
    TermId prev_log_term = _Message->prev_log_term;
    const std::vector<WW::RaftLogEntry> & other_entries = _Message->entries;
    LogIndex other_leader_commit = _Message->leader_commit;

    // 构造并设置响应消息
    RaftAppendEntriesResponseMessage append_entries_response_message;
    append_entries_response_message.seq = sequence_id;
    append_entries_response_message.from = _Id;
    append_entries_response_message.term = _Term;
    append_entries_response_message.success = false;

    // 1. 比较节点任期
    if (other_term < _Term) {
        // 任期小于自己，拒绝
        _Logger.debug("term: " + std::to_string(other_term) + " less than self: " + std::to_string(_Term) + ", refuse to append entries");
        _Inner_channel.push(std::move(append_entries_response_message));
        return;
    }

    if (other_term > _Term) {
        // 任期大于自己，更新状态
        _Logger.debug("term: " + std::to_string(other_term) + " larger than self: " + std::to_string(_Term) + ", switch to follower");
        _BecomeFollower(other_id, other_term);
        append_entries_response_message.term = _Term;
    }

    // 2. 判断指定日志是否存在
    if (!_LogMatch(prev_log_index, prev_log_term)) {
        // 没有这条日志，截断这之后的所有日志
        _TruncateAfter(prev_log_index);
        _Inner_channel.push(std::move(append_entries_response_message));
        _Logger.debug("log doesn't match at index: " + std::to_string(prev_log_index) + ", term: " + std::to_string(prev_log_term));
        _Logger.debug("which is index: " + std::to_string(prev_log_index) + ", term: " + std::to_string(_GetTermAt(prev_log_index)));
        return;
    }

    // 到这里已经收到合法报文，重置选举超时时间
    _ResetElectionDeadline();

    // 3. 同步日志
    if (!other_entries.empty()) {
        for (const RaftLogEntry & entry : other_entries) {
            _Logs.emplace_back(entry);
        }

        // 立即持久化
        _Persist();

        // 更新索引和任期
        _Last_log_index = _Base_index + _Logs.size() - 1;
        _Last_log_term = _Logs.back().getTerm();
    }

    // 4. 推进提交和应用
    if (other_leader_commit > _Last_commit_index) {
        _Last_commit_index = std::min(other_leader_commit, _Last_log_index);

        // 提交并应用可以提交的日志
        _ApplyCommitLogs();
    }

    // 心跳/同步成功
    append_entries_response_message.last_log_index = _Last_log_index;
    append_entries_response_message.success = true;

    _Inner_channel.push(std::move(append_entries_response_message));
}

void Raft::_HandleAppendEntriesResponse(const RaftAppendEntriesResponseMessage * _Message)
{
    if (_Role != RaftRole::Leader) {
        // 已经不是 Leader
        return;
    }

    // 读取上下文信息
    NodeId other_id = _Message->from;
    TermId other_term = _Message->term;
    LogIndex other_last_log_index = _Message->last_log_index;
    bool other_success = _Message->success;

    // 1. 比较节点任期
    if (other_term > _Term) {
        // 任期大于自己，退选
        _Logger.debug("candidate term: " + std::to_string(other_term) + " larger than self: " + std::to_string(_Term) + ", switch to follower");
        _BecomeFollower(-1, other_term);
        return;
    }

    // 找到该节点
    auto it = _Peers.begin();
    for (; it != _Peers.end(); ++it) {
        if (it->getId() == other_id) {
            break;
        }
    }

    if (it == _Peers.end()) {
        // 没找到这个节点
        _Logger.error("node: " + std::to_string(other_id) + "not fount");
        return;
    }

    RaftPeer & other_node = *it;

    // 2. 判断是否同步成功
    if (!other_success) {
        // 同步失败了，说明日志匹配失败了，调整后等待下一次同步
        LogIndex next_index = other_node.getNextIndex();
        if (next_index > 1) {
            other_node.setNextIndex(next_index - 1);
        }
        _Logger.debug("node: " + std::to_string(other_id) + " append entries failed, try to synchronise at index: " + std::to_string(other_node.getNextIndex()));
        return;
    }

    // 同步成功，更新索引
    other_node.setNextIndex(other_last_log_index + 1);
    other_node.setMatchIndex(other_last_log_index);

    // 3. 统计所有节点的 matchIndex
    std::vector<LogIndex> match_indexes;
    match_indexes.emplace_back(_Last_log_index);

    for (const RaftPeer & peer : _Peers) {
        match_indexes.emplace_back(peer.getMatchIndex());
    }

    std::sort(match_indexes.begin(), match_indexes.end());
    LogIndex majority_match = match_indexes[match_indexes.size() / 2];

    // 4. 如果该日志是当前任期的，同步
    if (_GetTermAt(majority_match) == _Term) {
        _Last_commit_index = majority_match;

        // 应用日志
        _ApplyCommitLogs();

        // 检查是否需要压缩快照
        _CheckIfNeedSnapshot();
    }
}

void Raft::_HandleInstallSnapshotRequest(const RaftInstallSnapshotRequestMessage * _Message)
{
    // 读取上下文信息
    TermId other_term = _Message->term;
    NodeId other_id = _Message->from;
    LogIndex other_last_included_index = _Message->last_included_index;
    TermId other_last_included_term = _Message->last_included_term;
    std::string other_snapshot = _Message->snapshot;
    SequenceType sequence_id = _Message->seq;

    // 构造并设置响应上下文
    RaftInstallSnapshotResponseMessage install_snapshot_response_message;
    install_snapshot_response_message.seq = sequence_id;

    // 1. 比较节点任期
    if (other_term < _Term) {
        // 任期小于自己，拒绝安装
        _Logger.debug("term: " + std::to_string(other_term) + " less than self: " + std::to_string(_Term) + ", refuse to install snapshot");
        install_snapshot_response_message.term = _Term;

        _Inner_channel.push(std::move(install_snapshot_response_message));
        return;
    }

    if (other_term > _Term) {
        // 任期大于自己，转换为 Follower
        _Logger.debug("term: " + std::to_string(other_term) + " larger than self: " + std::to_string(_Term) + ", switch to follower");
        _BecomeFollower(other_id, other_term);
        install_snapshot_response_message.term = _Term;
    }

    // 2. 比较快照新旧
    if (other_last_included_index < _Last_included_index) {
        // 快照比自己的旧，拒绝安装
        _Logger.debug("last included index: " + std::to_string(other_last_included_index) + " less than self: " + std::to_string(_Last_included_index) + ", refuse to install snapshot");
        install_snapshot_response_message.term = _Term;

        _Inner_channel.push(std::move(install_snapshot_response_message));
        return;
    }

    // 重置选举超时时间
    _ResetElectionDeadline();

    // 同意安装快照
    ApplySnapshotRequestMessage apply_snapshot_request_message;
    apply_snapshot_request_message.seq = sequence_id;
    apply_snapshot_request_message.last_included_index = other_last_included_index;
    apply_snapshot_request_message.last_included_term = other_last_included_term;
    apply_snapshot_request_message.snapshot = std::move(other_snapshot);

    _Inner_channel.push(std::move(apply_snapshot_request_message));
}

void Raft::_HandleInstallSnapshotResponse(const RaftInstallSnapshotResponseMessage * _Message)
{
    if (_Role != RaftRole::Leader) {
        // 已经不是 Leader
        return;
    }

    // 读取上下文信息
    NodeId other_id = _Message->from;
    TermId other_term = _Message->term;

    // 1. 比较节点任期
    if (other_term < _Term) {
        // 消息过期，忽略
        _Logger.warn("expired response, ignore");
        return;
    }

    if (other_term > _Term) {
        // 任期大于自己，退位
        _Logger.debug("term: " + std::to_string(other_term) + " larger than self: " + std::to_string(_Term) + ", switch to follower");
        _BecomeFollower(-1, other_term);
        return;
    }

    // 找到这个节点
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

    // 2. 更新节点索引
    other_node.setNextIndex(_Last_included_index + 1);
    other_node.setMatchIndex(_Last_included_index);
}

void Raft::_HandleApplyCommitLogs(const ApplyCommitLogsResponseMessage * _Message)
{
    // 读取上下文信息
    LogIndex last_commit_index = _Message->last_commit_index;    // 本次应用到哪一条日志
    _Last_applied_index = last_commit_index;
    _Is_applying = false;

    // 应用结束后判断是否要压缩快照
    _CheckIfNeedSnapshot();
}

void Raft::_HandleGenerateSnapshot(const GenerateSnapshotResponseMessage * _Message)
{
    // 读取上下文信息
    LogIndex last_included_index = _Message->last_applied_index;
    TermId last_included_term = _Message->last_applied_term;

    // 判断是否创建快照成功
    if (last_included_index == -1 && last_included_term == 0) {
        // 压缩快照失败
        return;
    }

    // 删除被快照压缩的日志
    _TruncateBefore(last_included_index + 1);

    // 更新索引和任期
    _Last_included_index = last_included_index;
    _Last_included_term = last_included_term;

    // 持久化
    _Persist();

    // 恢复状态
    _Is_snapshoting = false;
}

void Raft::_HandleApplySnapshot(const ApplySnapshotResponseMessage * _Message)
{
    // 读取上下文信息
    LogIndex last_included_index = _Message->last_included_index;
    TermId last_included_term = _Message->last_included_term;
    SequenceType sequence_id = _Message->seq;

    // 构造响应上下文
    RaftInstallSnapshotResponseMessage install_snapshot_response_message;
    install_snapshot_response_message.seq = sequence_id;

    // 判断是否安装快照成功
    if (last_included_index == -1 && last_included_term == 0) {
        // 压缩快照失败
        // 传入一个非法任期，使得服务器忽略该响应
        install_snapshot_response_message.term = 0;

        _Inner_channel.push(std::move(install_snapshot_response_message));
        return;
    }

    // 安装成功，删除被快照压缩的日志
    _TruncateBefore(last_included_index);

    // 更新索引和任期
    install_snapshot_response_message.term = _Term;

    _Last_included_index = last_included_index;
    _Last_included_term = last_included_term;

    _Inner_channel.push(std::move(install_snapshot_response_message));
}

void Raft::_HandleKVOperationRequest(const KVOperationRequestMessage * _Message)
{
    // 读取上下文信息
    RaftMessage::OperationType type = _Message->op_type;
    std::string uuid = _Message->uuid;
    std::string key = _Message->key;
    std::string value = _Message->value;
    SequenceType sequence_id = _Message->seq;

    // 构造上下文
    KVOperationResponseMessage kv_operation_response_message;
    kv_operation_response_message.uuid = std::move(uuid);
    kv_operation_response_message.key = std::move(key);
    kv_operation_response_message.value = std::move(value);
    kv_operation_response_message.success = false;
    kv_operation_response_message.seq = sequence_id;

    // 1. 判断身份
    if (_Role != RaftRole::Leader) {
        // 不是 Leader，拒绝操作
        kv_operation_response_message.leader_id = _Leader_id;
        // 直接通知应用层返回重定向响应
        _Inner_channel.push(std::move(kv_operation_response_message));
        return;
    }

    // 同意操作
    kv_operation_response_message.success = true;

    // 生成并添加一条日志
    if (type != RaftMessage::OperationType::GET) {
        std::string command;

        switch (type) {
        case RaftMessage::OperationType::PUT:
            kv_operation_response_message.op_type = RaftMessage::OperationType::PUT;
            command = "put " + kv_operation_response_message.key + " " + kv_operation_response_message.value;
            break;
        case RaftMessage::OperationType::UPDATE:
            kv_operation_response_message.op_type = RaftMessage::OperationType::UPDATE;
            command = "update " + kv_operation_response_message.key + " " + kv_operation_response_message.value;
            break;
        case RaftMessage::OperationType::DELETE:
            kv_operation_response_message.op_type = RaftMessage::OperationType::DELETE;
            command = "delete " + kv_operation_response_message.key;
            break;
        default:
            break;
        }

        // 插入日志条目
        RaftLogEntry new_entry(_Term, std::move(command), kv_operation_response_message.uuid, sequence_id);
        _Logs.emplace_back(std::move(new_entry));

        // 立即持久化
        _Persist();

        // 更新索引和任期
        ++_Last_log_index;
        _Last_log_term = _Term;

        // 发送日志同步请求
        _SendAppendEntries(false);
    } else {
        // 这里直接通知应用层操作（get），但实际上发送一个 no-op 心跳验证后操作更好
        kv_operation_response_message.op_type = RaftMessage::OperationType::GET;
        _Inner_channel.push(std::move(kv_operation_response_message));
    }
}

void Raft::_ApplyCommitLogs()
{
    if (_Is_applying) {
        return;
    }

    _Is_applying = true;

    // 构造应用上下文
    ApplyCommitLogsRequestMessage apply_commit_logs_request_message;
    apply_commit_logs_request_message.last_commit_index = _Last_commit_index;

    for (LogIndex i = _Last_applied_index + 1; i <= _Last_commit_index; ++i) {
        _Logger.debug("try to apply committed logs at index: " + std::to_string(i));
        const RaftLogEntry & entry = _GetLogAt(i);
        apply_commit_logs_request_message.entries.emplace_back(entry);
    }

    _Inner_channel.push(std::move(apply_commit_logs_request_message));
}

int Raft::_GetRandomTimeout(int _Timeout_min, int _Timeout_max) const
{
    // 创建全局随机数引擎
    static std::mt19937 rng(std::random_device{}());
    // 分布需要每次都创建
    std::uniform_int_distribution<int> dist(_Timeout_min, _Timeout_max);

    return dist(rng);
}

void Raft::_GenerateSnapshot()
{
    // 构造上下文
    GenerateSnapshotRequestMessage generate_snapshot_request_message;
    generate_snapshot_request_message.last_applied_index = _Last_applied_index;
    generate_snapshot_request_message.last_applied_term = _GetTermAt(_Last_applied_index);

    _Inner_channel.push(std::move(generate_snapshot_request_message));
}

void Raft::_CheckIfNeedSnapshot()
{
    if (_Is_snapshoting) {
        return;
    }

    _Is_snapshoting = true;

    // 设置压缩快照的阈值
    constexpr int SNAPSHOT_THRESHOLD = 3;     // for test

    if (_Last_applied_index - _Last_included_index >= SNAPSHOT_THRESHOLD) {
        // 超出阈值，准备压缩快照
        _GenerateSnapshot();
        return;
    }

    _Is_snapshoting = false;
}

void Raft::_ResetElectionDeadline()
{
    int election_timeout = _GetRandomTimeout(_Election_timeout_min, _Election_timeout_max);
    _Election_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(election_timeout);
}

void Raft::_ResetHeartbeatDeadline()
{
    _Heartbeat_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(_Heartbeat_timeout);
}

TermId Raft::_GetTermAt(LogIndex _Index) const
{
    if (_Index == _Last_included_index) {
        // 快照最后一条
        return _Last_included_term;
    }

    if (_Index < _Base_index || _Index > _Last_log_index) {
        // 不在范围内
        return 0;
    }

    return _Logs.at(_Index - _Base_index).getTerm();
}

bool Raft::_LogUpToDate(LogIndex _Last_index, TermId _Last_term) const
{
    if (_Last_term != _Last_log_term) {
        // 任期不同，比较任期
        return _Last_term > _Last_log_term;
    } else {
        // 任期相同，比较索引
        return _Last_index >= _Last_log_index;
    }
}

bool Raft::_LogMatch(LogIndex _Index, TermId _Term) const
{
    if (_Index == 0) {
        // 空日志
        return true;
    }

    if (_Index == _Last_included_index) {
        // 快照最后一条
        return _Last_included_term == _Term;
    }

    if (_Index < _Base_index || _Index > _Last_log_index) {
        // 不在范围内
        return false;
    }

    return _GetTermAt(_Index) == _Term;
}

void Raft::_TruncateAfter(LogIndex _Truncate_index)
{
    if (_Truncate_index < _Base_index || _Truncate_index > _Last_log_index) {
        return;
    }

    _Logs.resize(_Truncate_index - _Base_index);
}

void Raft::_TruncateBefore(LogIndex _Truncate_index)
{
    if (_Truncate_index <= _Base_index || _Truncate_index > _Last_log_index + 1) {
        return;
    }

    // 删除日志
    _Logs.erase(_Logs.begin(), _Logs.begin() + _Truncate_index - _Base_index);

    // 更新逻辑索引
    _Base_index = _Truncate_index;
}

const RaftLogEntry & Raft::_GetLogAt(LogIndex _Index) const
{
    if (_Index < _Base_index || _Index > _Last_log_index) {
        throw std::out_of_range("Index out of range in Raft::_GetLogAt");
    }

    return _Logs.at(_Index - _Base_index);
}

std::vector<RaftLogEntry> Raft::_GetLogFrom(LogIndex _Index) const
{
    std::vector<RaftLogEntry> tmp;

    if (_Index < _Base_index || _Index > _Last_log_index) {
        return tmp;
    }

    tmp.assign(_Logs.begin() + _Index - _Base_index, _Logs.end());

    return tmp;
}

void Raft::_Persist()
{
    // 构造一个持久化结构
    RaftPersistData persist_data;

    // 设置基本信息
    persist_data.set_term(_Term);
    persist_data.set_voted_for(_Voted_for);

    // 拷贝未压缩日志
    for (const RaftLogEntry & entry : _Logs) {
        RaftPersistLogEntry * persist_entry = persist_data.add_entries();
        persist_entry->set_term(entry.getTerm());
        persist_entry->set_command(entry.getCommand());
    }

    // 设置快照信息
    persist_data.set_last_included_index(_Last_included_index);
    persist_data.set_last_included_term(_Last_included_term);

    // 序列化
    std::string persist_str;
    if (!persist_data.SerializeToString(&persist_str)) {
        _Logger.error("persist data serialization failed");
        return;
    }

    // 写入文件
    std::string file_path = "raftnode_" + std::to_string(_Id) + ".persist";
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        _Logger.error("open raftnode persist file failed");
        return;
    }
    out.write(persist_str.data(), persist_str.size());
    out.close();

    _Logger.debug("raft data persisted");
}

bool Raft::loadPersist()
{
    // 读取文件
    std::string file_path = "raftnode_" + std::to_string(_Id) + ".persist";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        _Logger.error("open raftnode persist file failed");
        return false;
    }

    // 反序列化
    RaftPersistData persist_data;
    if (!persist_data.ParseFromIstream(&in)) {
        _Logger.error("persist data deserialization failed");
        return false;
    }
    in.close();

    // 加载持久化信息
    _Term = persist_data.term();
    _Voted_for = persist_data.voted_for();
    _Last_included_index = persist_data.last_included_index();
    _Last_included_term = persist_data.last_included_term();

    for (const RaftPersistLogEntry & persist_entry : persist_data.entries()) {
        _Logs.emplace_back(persist_entry.term(), persist_entry.command());
    }

    // 设置基本日志信息
    _Base_index = _Last_included_index + 1;
    _Last_log_index = _Base_index + _Logs.size() - 1;
    _Last_log_term = _GetTermAt(_Last_log_index);
    _Last_commit_index = _Last_included_index;
    _Last_applied_index = _Last_included_index;

    _Logger.debug("persist data loaded");

    return true;
}

} // namespace WW
