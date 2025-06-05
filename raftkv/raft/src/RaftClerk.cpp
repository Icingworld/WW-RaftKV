#include "RaftClerk.h"

#include <sstream>
#include <fstream>

#include <RaftPeer.h>
#include <RaftLogger.h>
#include <RaftSnapshot.pb.h>
#include <muduo/base/Logging.h>

namespace WW
{

RaftClerk::RaftClerk(NodeId _Id, const std::vector<RaftPeerNet> & _Peers)
    : _Raft(nullptr)
    , _Peers(_Peers)
    , _Timeout(0.01)
{
    // 初始化 Raft 和 clients
    std::vector<RaftPeer> peers;
    for (const RaftPeerNet & peer_net : _Peers) {
        peers.emplace_back(peer_net.getId());
        
        if (peer_net.getId() == _Id) {
            _Clients.emplace_back(nullptr);
            continue;
        }

        _Clients.emplace_back(new RaftRpcClient(&_Loop, peer_net.getIp(), peer_net.getPort()));
    }
    _Raft = new Raft(_Id, peers);

    // 初始化 Raft 服务端
    _Service.setRaftClerk(this);
    _Server = new RaftRpcServer(&_Loop, _Peers[_Id].getIp(), _Peers[_Id].getPort(), &_Service);

    // 初始化 Raft 操作服务端
    _Op_service.setRaftClerk(this);
    _Op_server = new RaftOperationServer(&_Loop, _Peers[_Id].getIp(), _Peers[_Id].getPort(), &_Op_service);

    // 设置 muduo 日志等级
    // muduo::Logger::setLogLevel(muduo::Logger::LogLevel::ERROR);

    // 从 Raft 持久化恢复
    if (_Raft->load()) {
        // 读取数据成功，安装快照
        _InstallSnapshotFromPersist();
    }
}

RaftClerk::~RaftClerk()
{
    delete _Raft;
    delete _Server;
    delete _Op_server;
}

void RaftClerk::_InstallSnapshotFromPersist()
{
    // 打开快照文件
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        ERROR("open snapshot file failed");
        return;
    }

    // 反序列化快照数据
    SnapshotData snapshot;
    if (!snapshot.ParseFromIstream(&in)) {
        ERROR("parse snapshot file failed");
        return;
    }

    // 应用快照数据到状态机
    for (const SnapshotEntry & entry : snapshot.kvs()) {
        if (!_KVStore.update(entry.key(), entry.value())) {
            ERROR("install snapshot failed");
            _KVStore.clear();
            return;
        }
    }

    DEBUG("install snapshot success");
}

void RaftClerk::run()
{
    // 启动 Raft 服务端
    _Server->start();

    // 启动 Raft 命令行服务端
    _Op_server->start();

    // 延迟 5s 连接所有节点
    DEBUG("client delaying");
    _Loop.runAfter(5.0, [this]() {
        DEBUG("client running");
        for (const RaftPeerNet peer_net : _Peers) {
            if (peer_net.getId() == _Raft->getId()) {
                continue;
            }

            RaftRpcClient * client = _Clients[peer_net.getId()];
            client->connect();
        }
    });

    // 延迟 10 秒启动定时器
    _Loop.runAfter(10.0, [this]() {
        // 启动后每 _Timeout 秒调用一次 _ClientWorking
        _Loop.runEvery(_Timeout, std::bind(&RaftClerk::_ClientWorking, this));
    });

    // 启动
    _Loop.loop();
}

void RaftClerk::stop()
{
    // 关闭定时器
}

void RaftClerk::_ClientWorking()
{
    // 推进 Raft 状态
    std::unique_lock<std::mutex> lock(_Mutex);
    _Raft->tick(static_cast<int>(_Timeout * 1000));

    // 读取 Raft 输出信息并处理
    const std::vector<RaftMessage> & messages = _Raft->readInnerMessage();
    lock.unlock();

    for (const RaftMessage & message : messages) {
        _HandleRaftMessageOut(message);
    }

    lock.lock();
    _Raft->clearInnerMessage();
    lock.unlock();
}

void RaftClerk::_HandleRaftMessageOut(const RaftMessage & _Message)
{
    switch (_Message.type) {
        case RaftMessage::MessageType::RequestVoteRequest:
            _SendRequestVoteRequest(_Message);
            break;
        case RaftMessage::MessageType::AppendEntriesRequest:
            _SendAppendEntriesRequest(_Message);
            break;
        case RaftMessage::MessageType::InstallSnapshotRequest:
            _SendInstallSnapshotRequest(_Message);
            break;
        case RaftMessage::MessageType::LogEntriesApply:
            _ApplyLogEntries(_Message);
            break;
        case RaftMessage::MessageType::TakeSnapshot:
            _GenerateSnapshot(_Message);
            break;
        case RaftMessage::MessageType::ApplySnapshot:
            _InstallSnapshot(_Message);
            break;
        default:
            break;
    }
}

void RaftClerk::_SendRequestVoteRequest(const RaftMessage & _Message)
{
    // 构造 Request 消息体
    RequestVoteRequest * vote_request = new RequestVoteRequest();
    vote_request->set_term(_Message.term);
    vote_request->set_candidate_id(_Message.from);
    vote_request->set_last_log_index(_Message.index);
    vote_request->set_last_log_term(_Message.log_term);

    // 发送请求
    DEBUG("send vote request to node: %d", _Message.to);
    RaftRpcClient * client = _Clients[_Message.to];
    client->RequestVote(*vote_request, std::bind(&RaftClerk::_HandleRequestVoteResponse, this, std::placeholders::_1));
}

void RaftClerk::_SendAppendEntriesRequest(const RaftMessage & _Message)
{
    // 获取上下文中的信息
    TermId this_term = _Message.term;
    NodeId this_id = _Message.from;
    NodeId other_id = _Message.to;
    LogIndex other_prev_log_index = _Message.index;
    TermId other_prev_log_term = _Message.log_term;
    LogIndex this_commit = _Message.commit;

    // 构造 Request 消息体
    AppendEntriesRequest * append_entries_request = new AppendEntriesRequest();
    append_entries_request->set_term(this_term);
    append_entries_request->set_leader_id(this_id);
    append_entries_request->set_prev_log_index(other_prev_log_index);
    append_entries_request->set_prev_log_term(other_prev_log_term);
    append_entries_request->set_leader_commit(this_commit);

    // 将日志添加到数组
    for (const RaftLogEntry & entry : _Message.entries) {
        WW::LogEntry * proto_entry = append_entries_request->add_entry();
        proto_entry->set_term(entry.getTerm());
        proto_entry->set_command(entry.getCommand());
    }

    // 发送请求
    RaftRpcClient * client = _Clients[_Message.to];
    client->AppendEntries(*append_entries_request, other_id, std::bind(&RaftClerk::_HandleAppendEntriesResponse, this, std::placeholders::_1, std::placeholders::_2));
}

void RaftClerk::_SendInstallSnapshotRequest(const RaftMessage & _Message)
{
    // 获取上下文中的信息
    TermId this_term = _Message.term;
    NodeId this_id = _Message.from;
    NodeId other_id = _Message.to;
    LogIndex this_last_include_index = _Message.index;
    TermId this_last_include_term = _Message.log_term;
    
    // 打开快照文件
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        ERROR("open snapshot file failed");
        return;
    }

    std::string this_snapshot((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // 构造 Request 消息体
    InstallSnapshotRequest * install_snapshot_request = new InstallSnapshotRequest();
    install_snapshot_request->set_term(this_term);
    install_snapshot_request->set_leader_id(this_id);
    install_snapshot_request->set_last_include_index(this_last_include_index);
    install_snapshot_request->set_last_include_term(this_last_include_term);
    install_snapshot_request->set_data(this_snapshot);

    // 发送请求
    RaftRpcClient * client = _Clients[_Message.to];
    client->InstallSnapshot(*install_snapshot_request, other_id, std::bind(&RaftClerk::_HandleInstallSnapshotResponse, this, std::placeholders::_1, std::placeholders::_2));
}

void RaftClerk::_ApplyLogEntries(const RaftMessage & _Message)
{
    // DEBUG("apply log entries");
    // 取出请求中的消息
    const std::vector<RaftLogEntry> & entries = _Message.entries;

    // 解析日志条目中的命令
    for (const RaftLogEntry & entry : entries) {
        std::string command = entry.getCommand();

        _ParseAndExecCommand(command);
    }
}

void RaftClerk::_GenerateSnapshot(const RaftMessage & _Message)
{
    // 取出上下文中的信息
    LogIndex this_last_applied_log_index = _Message.index;
    TermId this_last_applied_log_term = _Message.term;

    // 开始创建快照
    // 这里用 protobuf 来压缩快照
    SnapshotData snapshot;
    snapshot.set_last_applied_log_index(this_last_applied_log_index);
    snapshot.set_last_applied_log_term(this_last_applied_log_term);

    for (const auto & pair : _KVStore) {
        SnapshotEntry * entry = snapshot.add_kvs();
        entry->set_key(pair.first);
        entry->set_value(pair.second);
    }

    std::string snapshot_str = snapshot.SerializeAsString();

    // 写入文件
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Failed to open snapshot file for writing!" << std::endl;
        return;
    }
    out.write(snapshot_str.data(), snapshot_str.size());
    out.close();

    DEBUG("snapshot created");

    // 写入成功，通知 Raft 截断日志
    RaftMessage message;
    message.type = RaftMessage::MessageType::ApplySnapshot;
    message.index = this_last_applied_log_index;
    message.log_term = this_last_applied_log_term;

    // 通知 Raft
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_InstallSnapshot(const RaftMessage & _Message)
{
    // 取出快照上下文中的信息
    LogIndex this_index = _Message.index;
    TermId this_log_term = _Message.log_term;
    std::string snapshot_str = _Message.snapshot;

    // 先保存快照到本地
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Failed to open snapshot file for writing!" << std::endl;
        return;
    }
    out.write(snapshot_str.data(), snapshot_str.size());
    out.close();

    DEBUG("snapshot saved");

    // 解析快照
    SnapshotData snapshot;
    if (!snapshot.ParseFromString(snapshot_str)) {
        ERROR("parse snapshot failed");
        return;
    }

    // 清空状态机，准备应用快照
    _KVStore.clear();

    // 安装快照
    for (const SnapshotEntry & entry : snapshot.kvs()) {
        std::string key = entry.key();
        std::string value = entry.value();

        // put 应该也一样
        if (!_KVStore.update(key, value)) {
            // 安装快照失败了，等待 Leader 下一次同步重试
            ERROR("install snapshot failed");
            _KVStore.clear();
            return;
        }
    }

    DEBUG("install snapshot success");

    // 安装成功，通知 Raft
    RaftMessage message;
    message.type = RaftMessage::MessageType::ApplySnapshot;
    message.index = this_index;
    message.log_term = this_log_term;

    // 通知 Raft
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_HandleRequestVoteRequest(const RequestVoteRequest & _Request, RequestVoteResponse & _Response)
{
    // 取出请求中的消息
    TermId other_term = _Request.term();
    NodeId other_id = _Request.candidate_id();
    LogIndex other_last_log_index = _Request.last_log_index();
    TermId other_last_log_term = _Request.last_log_term();

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
    // 取出请求中的信息
    TermId other_term = _Request.term();
    NodeId other_id = _Request.leader_id();
    LogIndex other_prev_log_index = _Request.prev_log_index();
    LogIndex other_prev_log_term = _Request.prev_log_term();
    const auto & other_entries = _Request.entry();
    LogIndex other_commit = _Request.leader_commit();

    // 构造消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::AppendEntriesRequest;

    // 判断条目是否为空
    if (!other_entries.empty()) {
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
    // 取出响应中的信息
    TermId other_term = _Response.term();
    bool other_success = _Response.success();
    LogIndex other_index = _Response.index();

    // 构造消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::AppendEntriesResponse;
    message.term = other_term;
    message.from = _Id;
    message.index = other_index;
    message.reject = !other_success;

    // 传递给 Raft 状态机
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_HandleInstallSnapshotRequest(const InstallSnapshotRequest & _Request, InstallSnapshotResponse & _Response)
{
    // 取出请求中的信息
    TermId other_term = _Request.term();
    NodeId other_id = _Request.leader_id();
    LogIndex other_last_include_index = _Request.last_include_index();
    TermId other_last_include_term = _Request.last_include_term();
    std::string other_snapshot = _Request.data();

    // 构造消息
    RaftMessage message;
    message.term = other_term;
    message.from = other_id;
    message.index = other_last_include_index;
    message.log_term = other_last_include_term;
    message.snapshot = other_snapshot;

    // 传入 Raft
    std::unique_lock<std::mutex> lock(_Mutex);
    _Raft->step(message);

    // 取出当此响应
    const RaftMessage & out_message = _Raft->readOutterMessage();
    lock.unlock();

    // 读取消息中的信息
    TermId this_term = out_message.term;

    // 设置响应
    _Response.set_term(this_term);
}

void RaftClerk::_HandleInstallSnapshotResponse(NodeId _Id, const InstallSnapshotResponse & _Response)
{
    // 取出响应中的信息
    TermId other_term = _Response.term();

    // 构造消息
    RaftMessage message;
    message.term = other_term;
    message.from = _Id;

    // 传递给 Raft 状态机
    std::lock_guard<std::mutex> lock(_Mutex);
    _Raft->step(message);
}

void RaftClerk::_ParseAndExecCommand(const std::string & _Command)
{
    // 解析命令
    std::istringstream iss(_Command);
    std::string op;
    iss >> op;

    if (op == "put") {
        // 1. 插入操作
        std::string key, value;
        iss >> key >> value;
        if (!key.empty() && !value.empty()) {
            if (!_KVStore.put(key, value)) {
                ERROR("kvstore put key: %s, value: %s failed", key.c_str(), value.c_str());
            } else {
                DEBUG("kvstore put key: %s, value: %s success", key.c_str(), value.c_str());
            }
        } else {
            ERROR("invalid put command");
        }
    } else if (op == "update") {
        // 2. 更新操作
        std::string key, value;
        iss >> key >> value;
        if (!key.empty() && !value.empty()) {
            if (!_KVStore.update(key, value)) {
                ERROR("kvstore update key: %s, value: %s failed", key.c_str(), value.c_str());
            } else {
                DEBUG("kvstore update key: %s, value: %s success", key.c_str(), value.c_str());
            }
        } else {
            ERROR("invalid update command");
        }
    } else if (op == "remove") {
        // 3. 删除操作
        std::string key;
        iss >> key;
        if (!key.empty()) {
            if (!_KVStore.remove(key)) {
                ERROR("kvstore remove key: %s failed", key.c_str());
            } else {
                DEBUG("kvstore remove key: %s success", key.c_str());
            }
        } else {
            ERROR("invalid remove command");
        }
    }
    else {
        ERROR("unknown kvstore command: %s", op.c_str());
    }
}

void RaftClerk::_HandleOperateRaftRequest(const RaftOperationRequest & _Request, RaftOperationResponse & _Response)
{
    DEBUG("receive operation request");
    // 取出请求中的信息
    CommandType type = _Request.type();
    std::string key = _Request.key();
    std::string value = _Request.value();

    // 构造上下文消息
    RaftMessage message;
    message.type = RaftMessage::MessageType::OperationRequest;

    switch (type) {
        case CommandType::PUT:
            message.op_type = RaftMessage::OperationType::PUT;
            message.command = "put " + key + " " + value;
            DEBUG("command: %s", message.command.c_str());
            break;
        case CommandType::UPDATE:
            message.op_type = RaftMessage::OperationType::UPDATE;
            message.command = "update " + key + " " + value;
            DEBUG("command: %s", message.command.c_str());
            break;
        case CommandType::REMOVE:
            message.op_type = RaftMessage::OperationType::REMOVE;
            message.command = "remove " + key;
            DEBUG("command: %s", message.command.c_str());
            break;
        case CommandType::GET:
            message.op_type = RaftMessage::OperationType::GET;
            message.command = "get " + key;
            DEBUG("command: %s", message.command.c_str());
            break;
        default:
            break;
    }

    std::unique_lock<std::mutex> lock(_Mutex);
    _Raft->step(message);

    // 取出响应上下文消息
    const RaftMessage & out_message = _Raft->readOutterMessage();
    lock.unlock();

    // 读取消息中的信息
    bool this_reject = out_message.reject;
    NodeId leader_id;

    if (!this_reject) {
        leader_id = out_message.from;
    } else {
        leader_id = out_message.to;
    }

    if (!this_reject) {
        // 是 Leader，允许 get 操作
        if (type == CommandType::GET) {
            value = _KVStore.get(key);
        }
    }

    // 组织响应消息
    _Response.set_success(!this_reject);
    _Response.set_value(value);

    if (this_reject) {
        // 不是 Leader，返回 Leader 的地址
        _Response.set_is_leader(!this_reject);

        // 取出对应节点地址
        const RaftPeerNet & peer = _Peers[leader_id];
        _Response.set_leader_address(peer.getIp() + ":" + peer.getPort());
    }
}

} // namespace WW
