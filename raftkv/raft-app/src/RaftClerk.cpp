#include "RaftClerk.h"

#include <sstream>
#include <fstream>

#include <ConsoleSink.h>
#include <muduo/base/Logging.h>
#include <RaftSnapshot.pb.h>

namespace WW
{

RaftClerk::RaftClerk(NodeId _Id, const std::vector<RaftPeerNet> & _Peers)
    : _Raft(nullptr)
    , _Peers(_Peers)
    , _KVStore()
    , _Clients()
    , _Event_loop_client(nullptr)
    , _Event_loop_thread_pool(nullptr)
    , _Rpc_service()
    , _Rpc_server(nullptr)
    , _KVOperation_service()
    , _KVOperation_server(nullptr)
    , _Running(false)
    , _Message_thread()
    , _Wait_ms(-1)
    , _Pending_requests()
    , _Logger(Logger::getSyncLogger("RaftClerk"))
{
    // 设置日志参数
    _Logger.setLevel(LogLevel::Debug);
    std::shared_ptr<ConsoleSink> console_sink = std::make_shared<ConsoleSink>();
    _Logger.addSink(console_sink);

    // 设置 Rpc 日志参数
    Logger & rpc_logger = Logger::getSyncLogger("RaftRpc");
    rpc_logger.setLevel(LogLevel::Debug);
    rpc_logger.addSink(console_sink);

    // 设置 muduo 日志等级
    muduo::Logger::setLogLevel(muduo::Logger::LogLevel::ERROR);

    // 初始化 EventLoop
    _Event_loop_client = std::make_shared<muduo::net::EventLoop>();

    // 初始化 EventLoopThreadPool
    _Event_loop_thread_pool = std::unique_ptr<muduo::net::EventLoopThreadPool>(
        new muduo::net::EventLoopThreadPool(_Event_loop_client.get(), "RaftRpcThreadPool")
    );
    _Event_loop_thread_pool->setThreadNum(2);
    _Event_loop_thread_pool->start();

    // 初始化 RaftPeer 和 client
    std::vector<RaftPeer> peers;
    for (const RaftPeerNet & peer_net : _Peers) {
        peers.emplace_back(peer_net.getId());

        if (peer_net.getId() == _Id) {
            _Clients.emplace_back(nullptr);
            continue;
        }

        // 获取一个 EventLoop
        muduo::net::EventLoop * client_loop = _Event_loop_thread_pool->getNextLoop();
        _Clients.emplace_back(new RaftRpcClient(client_loop, peer_net.getIp(), peer_net.getPort()));
    }

    // 初始化 Raft
    _Raft = std::unique_ptr<Raft>(
        new Raft(_Id, peers)
    );

    // 注册回调函数到 Raft Service
    _Rpc_service.registerRequestVoteCallback(std::bind(
        &RaftClerk::_HandleRequestVoteRequest, this, std::placeholders::_1, std::placeholders::_2
    ));

    _Rpc_service.registerAppendEntriesCallback(std::bind(
        &RaftClerk::_HandleAppendEntriesRequest, this, std::placeholders::_1, std::placeholders::_2
    ));

    _Rpc_service.registerInstallSnapshotCallback(std::bind(
        &RaftClerk::_HandleInstallSnapshotRequest, this, std::placeholders::_1, std::placeholders::_2
    ));

    // 初始化 Raft 服务端
    _Rpc_server = std::unique_ptr<RaftRpcServer>(
        new RaftRpcServer(_Event_loop_client, _Peers[_Id].getIp(), _Peers[_Id].getPort(), &_Rpc_service)
    );

    // 注册回调函数到 KVOperation Service
    _KVOperation_service.registerExecuteCallback(std::bind(
        &RaftClerk::_HandleKVOperationRequest, this, std::placeholders::_1, std::placeholders::_2
    ));

    // 初始化 KVOperation 服务端
    _KVOperation_server = std::unique_ptr<KVOperationServer>(
        new KVOperationServer(_Event_loop_client, _Peers[_Id].getIp(), _Peers[_Id].getKVPort(), &_KVOperation_service)
    );

    // 从持久化和快照初始化 Raft
    if (_Raft->loadPersist()) {
        // 安装本地快照
        _InstallSnapshotFromPersist();
    }
}

RaftClerk::~RaftClerk()
{
    _Event_loop_client->quit();
}

void RaftClerk::start()
{
    // 延迟 5s 连接所有节点
    _Event_loop_client->runAfter(5.0, [this]() {
        // 连接所有节点
        for (const RaftPeerNet peer_net : _Peers) {
            if (peer_net.getId() == _Raft->getId()) {
                continue;
            }

            RaftRpcClient * client = _Clients[peer_net.getId()];
            _Logger.debug("connecting to node: " + std::to_string(peer_net.getId()));
            client->connect();
        }

        // 启动消息队列
        _Running.store(true);
        _Message_thread = std::thread(&RaftClerk::_GetInnerMessage, this);

        _Raft->startMessage();
    });

    // 延迟 10 秒启动 Raft
    _Event_loop_client->runAfter(10.0, [this]() {
        // 启动 Raft
        _Raft->start();
    });

    // 启动 Raft 服务端
    _Rpc_server->start();
    // 启动 KVOperation 服务端
    _KVOperation_server->start();

    // 启动循环
    _Event_loop_client->loop();
}

void RaftClerk::stop()
{
    _Running.store(false);

    _Raft->stop();

    if (_Message_thread.joinable()) {
        _Message_thread.join();
    }
}

void RaftClerk::_GetInnerMessage()
{
    while (_Running.load()) {
        RaftMessage message;
        if (_Raft->readReady(message, _Wait_ms)) {
            _HandleMessage(message);
        }
    }
}

void RaftClerk::_HandleMessage(const RaftMessage & _Message)
{
    switch (_Message.type) {
    case RaftMessage::MessageType::RequestVoteRequest:
        _SendRequestVoteRequest(_Message);
        break;
    case RaftMessage::MessageType::RequestVoteResponse:
        _SendRequestVoteResponse(_Message);
        break;
    case RaftMessage::MessageType::AppendEntriesRequest:
        _SendAppendEntriesRequest(_Message);
        break;
    case RaftMessage::MessageType::AppendEntriesResponse:
        _SendAppendEntriesResponse(_Message);
        break;
    case RaftMessage::MessageType::InstallSnapshotRequest:
        _SendInstallSnapshotRequest(_Message);
        break;
    case RaftMessage::MessageType::InstallSnapshotResponse:
        _SendInstallSnapshotResponse(_Message);
        break;
    case RaftMessage::MessageType::KVOPerationResponse:
        _SendKVOperationResponse(_Message);
        break;
    case RaftMessage::MessageType::ApplyCommitLogs:
        _ApplyCommitLogs(_Message);
        break;
    case RaftMessage::MessageType::ApplySnapshot:
        _ApplySnapshot(_Message);
        break;
    case RaftMessage::MessageType::GenerateSnapshot:
        _GenerateSnapshot(_Message);
        break;
    default:
        break;
    }
}

void RaftClerk::_SendRequestVoteRequest(const RaftMessage & _Message)
{
    // 读取上下文信息
    TermId this_term = _Message.term;
    NodeId this_id = _Message.from;
    NodeId other_id = _Message.to;
    LogIndex this_last_log_index = _Message.index;
    TermId this_last_log_term = _Message.log_term;

    // _Logger.debug("to node: " + std::to_string(other_id));

    // 构造 Request 消息体
    RequestVoteRequest * vote_request = new RequestVoteRequest();
    vote_request->set_term(this_term);
    vote_request->set_candidate_id(this_id);
    vote_request->set_last_log_index(this_last_log_index);
    vote_request->set_last_log_term(this_last_log_term);

    // 发送请求
    RaftRpcClient * client = _Clients[other_id];
    client->RequestVote(vote_request, [this](const RequestVoteResponse * _Response, google::protobuf::RpcController * _Controller) {
        _HandleRequestVoteResponse(_Response, _Controller);
    });
}

void RaftClerk::_SendRequestVoteResponse(const RaftMessage & _Message)
{
    // 读取上下文信息
    TermId this_term = _Message.term;
    bool this_success = _Message.success;
    uint64_t sequence_id = _Message.seq;
    // _Logger.debug("_SendRequestVoteResponse seq: " + std::to_string(sequence_id));

    // 找到回调函数
    RaftRpcServerClosure * done = _Pending_requests[sequence_id];
    // 取出 Response 消息体
    RequestVoteResponse * response = dynamic_cast<RequestVoteResponse *>(done->response());

    // 设置 Response
    response->set_term(this_term);
    response->set_vote_granted(this_success);

    // 触发回调
    done->Run();

    // 从等待列表中删除
    _Pending_requests.erase(sequence_id);
}

void RaftClerk::_SendAppendEntriesRequest(const RaftMessage & _Message)
{
    // 读取上下文信息
    TermId this_term = _Message.term;
    NodeId this_id = _Message.from;
    NodeId other_id = _Message.to;
    LogIndex other_prev_log_index = _Message.index;
    TermId other_prev_log_term = _Message.log_term;
    LogIndex this_commit = _Message.commit;

    // _Logger.debug("this id: " + std::to_string(this_id) + ", term: " + std::to_string(this_term));

    // 构造 Request 消息体
    AppendEntriesRequest * append_entries_request = new AppendEntriesRequest();
    append_entries_request->set_term(this_term);
    append_entries_request->set_leader_id(this_id);
    append_entries_request->set_prev_log_index(other_prev_log_index);
    append_entries_request->set_prev_log_term(other_prev_log_term);
    append_entries_request->set_commit_index(this_commit);

    // 将日志条目添加到请求中
    for (const RaftLogEntry & entry : _Message.entries) {
        WW::LogEntry * proto_entry = append_entries_request->add_entries();
        proto_entry->set_term(entry.getTerm());
        proto_entry->set_command(entry.getCommand());
    }

    // 发送请求
    RaftRpcClient * client = _Clients[other_id];
    client->AppendEntries(append_entries_request, [this, other_id](const AppendEntriesResponse * _Response, google::protobuf::RpcController * _Controller) {
        _HandleAppendEntriesResponse(other_id, _Response, _Controller);
    });
}

void RaftClerk::_SendAppendEntriesResponse(const RaftMessage & _Message)
{
    // 取出上下文信息
    TermId this_term = _Message.term;
    LogIndex this_index = _Message.index;
    bool this_success = _Message.success;
    uint64_t sequence_id = _Message.seq;

    // 找到回调函数
    RaftRpcServerClosure * done = _Pending_requests[sequence_id];
    // 取出 Response 消息体
    AppendEntriesResponse * response = dynamic_cast<AppendEntriesResponse *>(done->response());

    // 设置 Response
    response->set_term(this_term);
    response->set_success(this_success);
    response->set_index(this_index);

    // 触发回调
    done->Run();

    // 从等待列表中删除
    _Pending_requests.erase(sequence_id);
}

void RaftClerk::_SendInstallSnapshotRequest(const RaftMessage & _Message)
{
    // 读取上下文信息
    TermId this_term = _Message.term;
    NodeId this_id = _Message.from;
    NodeId other_id = _Message.to;
    LogIndex this_last_included_index = _Message.index;
    TermId this_last_included_term = _Message.log_term;

    // 加载快照内容
    std::string file_path = "snapshot_" + std::to_string(this_id) + ".snapshot";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        _Logger.error("open snapshot file failed");
        return;
    }
    std::string snapshot_data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // 构造 InstallSnapshot 消息体
    InstallSnapshotRequest * request = new InstallSnapshotRequest();
    request->set_term(this_term);
    request->set_leader_id(this_id);
    request->set_last_included_index(this_last_included_index);
    request->set_last_included_term(this_last_included_term);
    request->set_data(snapshot_data);

    // 发送请求
    RaftRpcClient * client = _Clients[other_id];
    client->InstallSnapshot(request, [this, other_id](const InstallSnapshotResponse * _Response, google::protobuf::RpcController * _Controller) {
        _HandleInstallSnapshotResponse(other_id, _Response, _Controller);
    });
}

void RaftClerk::_SendInstallSnapshotResponse(const RaftMessage & _Message)
{
    // 读取上下文信息
    TermId this_term = _Message.term;
    uint64_t sequence_id = _Message.seq;

    // 找到回调函数
    RaftRpcServerClosure * done = _Pending_requests[sequence_id];
    // 取出 Response 消息体
    InstallSnapshotResponse * response = dynamic_cast<InstallSnapshotResponse *>(done->response());

    // 设置 response
    response->set_term(this_term);

    // 触发回调
    done->Run();

    // 从等待列表中删除
    _Pending_requests.erase(sequence_id);
}

void RaftClerk::_SendKVOperationResponse(const RaftMessage & _Message)
{
    // 读取上下文信息
    RaftMessage::OperationType type = _Message.op_type;
    std::string uuid = _Message.uuid;
    NodeId leader_id = _Message.to;
    std::string key = _Message.key;
    std::string value = _Message.value;
    bool success = _Message.success;
    uint64_t sequence_id = _Message.seq;

    // 找到回调函数
    std::map<uint64_t, RaftRpcServerClosure *> & client_requests = _Pending_kv_requests[uuid];
    RaftRpcServerClosure * done = client_requests[sequence_id];
    // 取出 Response 消息体
    KVOperationResponse * response = dynamic_cast<KVOperationResponse *>(done->response());

    // 只有 get 操作和重定向直接返回

    if (success) {
        // 同意操作
        switch (type) {
        case RaftMessage::OperationType::GET:
            // 执行读取操作
            value = _KVStore.get(key);
            if (value == "") {
                // 没找到这个键
                _Logger.warn("key: " + key + " not found");
                response->set_status_code(KVOperationResponse_StatusCode_NOT_FOUND);
            } else {
                _Logger.debug("get key: " + key + ", value: " + value);
                response->set_status_code(KVOperationResponse_StatusCode_SUCCESS);
                response->set_payload(value);
            }
            break;
        default:
            break;
        }
    } else {
        // 不是 Leader，拒绝操作
        _Logger.warn("not leader, redirect to node: " + leader_id);
        response->set_status_code(KVOperationResponse_StatusCode_REDIRECT);

        // 根据 Leader ID 查找地址
        const RaftPeerNet & peer = _Peers[leader_id];
        response->set_address(peer.getIp() + ":" + peer.getKVPort());
    }

    // 触发回调
    done->Run();

    // 从等待列表中删除
    client_requests.erase(sequence_id);
}

void RaftClerk::_ApplyCommitLogs(const RaftMessage & _Message)
{
    // 读取上下文中的信息
    LogIndex last_commit_index = _Message.index;
    const std::vector<RaftLogEntry> & entries = _Message.entries;

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::ApplyCommitLogs;

    // 开始解析并应用日志条目
    for (const RaftLogEntry & entry : entries) {
        // 读取日志条目中的信息
        const std::string & uuid = entry.getUUID();
        uint64_t sequence_id = entry.getSequenceID();
        TermId term = entry.getTerm();
        const std::string & command = entry.getCommand();

        // 准备设置状态
        KVOperationResponse::StatusCode status_code = KVOperationResponse_StatusCode_DEFAULT;

        // 解析命令
        std::istringstream iss(command);
        std::string op;
        iss >> op;

        if (op == "put") {
            // 1. 插入操作
            std::string key, value;
            iss >> key >> value;
            if (!key.empty() && !value.empty()) {
                if (!_KVStore.put(key, value)) {
                    status_code = KVOperationResponse_StatusCode_INTERNAL_ERROR;
                    _Logger.error("kvstore put key: " + key + ", value: " + value + " failed");
                } else {
                    status_code = KVOperationResponse_StatusCode_CREATED;
                    _Logger.debug("kvstore put key: " + key + ", value: " + value + " success");
                }
            } else {
                _Logger.error("invalid put command");
            }
        } else if (op == "update") {
            // 2. 更新操作
            std::string key, value;
            iss >> key >> value;
            if (!key.empty() && !value.empty()) {
                if (!_KVStore.update(key, value)) {
                    status_code = KVOperationResponse_StatusCode_INTERNAL_ERROR;
                    _Logger.error("kvstore update key: " + key + ", value: " + value + " failed");
                } else {
                    status_code = KVOperationResponse_StatusCode_CREATED;
                    _Logger.debug("kvstore update key: " + key + ", value: " + value + " success");
                }
            } else {
                _Logger.error("invalid update command");
            }
        } else if (op == "delete") {
            // 3. 删除操作
            std::string key;
            iss >> key;
            if (!key.empty()) {
                if (!_KVStore.remove(key)) {
                    status_code = KVOperationResponse_StatusCode_INTERNAL_ERROR;
                    _Logger.error("kvstore remove key: " + key + " failed");
                } else {
                    status_code = KVOperationResponse_StatusCode_CREATED;
                    _Logger.debug("kvstore remove key: " + key + " success");
                }
            } else {
                _Logger.error("invalid remove command");
            }
        }
        else {
            _Logger.error("unknown kvstore operation: " + op);
        }

        // 判断是否需要发送响应
        if (!uuid.empty() && sequence_id != 0) {
            // 找到回调函数
            std::map<uint64_t, RaftRpcServerClosure *> & client_requests = _Pending_kv_requests[uuid];
            RaftRpcServerClosure * done = client_requests[sequence_id];
            // 取出 Response 消息体
            KVOperationResponse * response = dynamic_cast<KVOperationResponse *>(done->response());

            // 设置状态码
            response->set_status_code(status_code);

            // 调用回调函数
            done->Run();

            // 清除
            client_requests.erase(sequence_id);
        }
    }

    // 应用成功
    message.index = last_commit_index;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_ApplySnapshot(const RaftMessage & _Message)
{
    // 读取上下文信息
    LogIndex last_included_index = _Message.index;
    TermId last_included_term = _Message.log_term;
    std::string snapshot = _Message.snapshot;
    uint64_t sequence_id = _Message.seq;

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::ApplySnapshot;
    message.seq = sequence_id;

    // 先将快照保存到本地
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        _Logger.error("open snapshot file failed");
        // 标记为失败
        message.index = -1;
        message.log_term = 0;

        // 传入 Raft
        _Raft->step(message);
        return;
    }
    out.write(snapshot.data(), snapshot.size());
    out.close();

    _Logger.debug("snapshot saved to file");

    // 解析快照内容
    RaftSnapshotData snapshot_data;
    if (!snapshot_data.ParseFromString(snapshot)) {
        _Logger.error("snapshot data serialization failed");
        // 标记为失败
        message.index = -1;
        message.log_term = 0;

        // 传入 Raft
        _Raft->step(message);
        return;
    }

    // 安装快照
    _KVStore.clear();

    for (const RaftSnapshotEntry & entry : snapshot_data.entries()) {
        std::string key = entry.key();
        std::string value = entry.value();

        if (!_KVStore.update(key, value)) {
            // 安装快照失败了，等待 Leader 下一次同步重试
            _Logger.error("install snapshot failed");
            _KVStore.clear();

            // 标记为失败
            message.index = -1;
            message.log_term = 0;

            // 传入 Raft
            _Raft->step(message);
            return;
        }
    }

    // 安装成功
    message.index = last_included_index;
    message.log_term = last_included_term;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_GenerateSnapshot(const RaftMessage & _Message)
{
    // 读取上下文信息
    LogIndex last_applied_index = _Message.index;
    TermId last_applied_term = _Message.log_term;

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::GenerateSnapshot;

    // 开始创建快照
    RaftSnapshotData snapshot_data;
    for (const std::pair<const std::string, std::string> & pair : _KVStore) {
        RaftSnapshotEntry * entry = snapshot_data.add_entries();
        entry->set_key(pair.first);
        entry->set_value(pair.second);
    }

    // 序列化
    std::string snapshot_str;
    if (!snapshot_data.SerializeToString(&snapshot_str)) {
        _Logger.error("snapshot data serialization failed");
        // 标记为失败
        message.index = -1;
        message.log_term = 0;

        // 传入 Raft
        _Raft->step(message);
        return;
    }

    // 写入文件
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        _Logger.error("open snapshot file failed");
        // 标记为失败
        message.index = -1;
        message.log_term = 0;

        // 传入 Raft
        _Raft->step(message);
        return;
    }
    out.write(snapshot_str.data(), snapshot_str.size());
    out.close();

    _Logger.debug("create snapshot success");

    // 标记为成功
    message.index = last_applied_index;
    message.log_term = last_applied_term;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_InstallSnapshotFromPersist()
{
    // 读取文件
    std::string file_path = "snapshot_" + std::to_string(_Raft->getId()) + ".snapshot";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        _Logger.error("open raftnode persist file failed");
        return;
    }

    // 反序列化
    RaftSnapshotData snapshot_data;
    if (!snapshot_data.ParseFromIstream(&in)) {
        _Logger.error("persist data deserialization failed");
        return;
    }
    in.close();

    // 应用到 KV 储存
    for (const RaftSnapshotEntry & entry : snapshot_data.entries()) {
        std::string key = entry.key();
        std::string value = entry.value();

        if (!_KVStore.update(key, value)) {
            // 严重错误，可以主动引发程序崩溃重启
            // 或者通知 Raft 重新初始化，等待同步
            _Logger.fatal("install snapshot from persist failed");
        }
    }

    // 安装成功
    _Logger.debug("install snapshot from persist success");
}

void RaftClerk::_HandleRequestVoteRequest(const RequestVoteRequest * _Request, google::protobuf::Closure * _Done)
{
    // 取出请求中的信息
    TermId other_term = _Request->term();
    NodeId other_id = _Request->candidate_id();
    LogIndex other_last_log_index = _Request->last_log_index();
    TermId other_last_log_term = _Request->last_log_term();

    // _Logger.debug("_HandleRequestVoteRequest id: " + std::to_string(other_id) + " term: " + std::to_string(other_term));

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::RequestVoteRequest;
    message.from = other_id;
    message.term = other_term;
    message.index = other_last_log_index;
    message.log_term = other_last_log_term;

    // 取出 Done 中的 sequence_id 并注册
    RaftRpcServerClosure * done = dynamic_cast<RaftRpcServerClosure *>(_Done);
    uint64_t sequence_id = done->sequenceId();
    // _Logger.debug("_HandleRequestVoteRequest seq: " + std::to_string(sequence_id));
    message.seq = sequence_id;
    _Pending_requests[sequence_id] = done;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_HandleRequestVoteResponse(const RequestVoteResponse * _Response, google::protobuf::RpcController * _Controller)
{
    // 取出响应中的消息
    TermId other_term = _Response->term();
    bool other_vote_granted = _Response->vote_granted();

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::RequestVoteResponse;
    message.term = other_term;
    message.success = other_vote_granted;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_HandleAppendEntriesRequest(const AppendEntriesRequest * _Request, google::protobuf::Closure * _Done)
{
    // 取出请求中的信息
    TermId other_term = _Request->term();
    NodeId other_id = _Request->leader_id();
    LogIndex other_prev_log_index = _Request->prev_log_index();
    LogIndex other_prev_log_term = _Request->prev_log_term();
    const auto & other_entries = _Request->entries();
    LogIndex other_commit = _Request->commit_index();

    // _Logger.debug("append entries from id: " + std::to_string(other_id) + ", term: " + std::to_string(other_term));

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::AppendEntriesRequest;
    message.term = other_term;
    message.from = other_id;
    message.index = other_prev_log_index;
    message.log_term = other_prev_log_term;
    message.commit = other_commit;

    // 添加日志条目
    for (const auto & entry : other_entries) {
        _Logger.debug("command: " + entry.command());
        message.entries.emplace_back(entry.term(), entry.command());
    }

    // 取出 Done 中的 sequence_id 并注册
    RaftRpcServerClosure * done = dynamic_cast<RaftRpcServerClosure *>(_Done);
    uint64_t sequence_id = done->sequenceId();
    message.seq = sequence_id;
    _Pending_requests[sequence_id] = done;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_HandleAppendEntriesResponse(NodeId _Id, const AppendEntriesResponse * _Response, google::protobuf::RpcController * _Controller)
{
    // 取出响应中的消息
    TermId other_term = _Response->term();
    bool other_success = _Response->success();
    LogIndex other_index = _Response->index();

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::AppendEntriesResponse;
    message.from = _Id;
    message.term = other_term;
    message.success = other_success;
    message.index = other_index;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_HandleInstallSnapshotRequest(const InstallSnapshotRequest* _Request, google::protobuf::Closure * _Done)
{
    // 读取请求中的信息
    TermId other_term = _Request->term();
    NodeId other_id = _Request->leader_id();
    LogIndex other_last_included_index = _Request->last_included_index();
    TermId other_last_included_term = _Request->last_included_term();
    std::string other_snapshot = _Request->data();

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::InstallSnapshotRequest;
    message.from = other_id;
    message.term = other_term;
    message.index = other_last_included_index;
    message.log_term = other_last_included_term;
    message.snapshot = other_snapshot;

    // 取出 Done 中的 sequence_id 并注册
    RaftRpcServerClosure * done = dynamic_cast<RaftRpcServerClosure *>(_Done);
    uint64_t sequence_id = done->sequenceId();
    message.seq = sequence_id;
    _Pending_requests[sequence_id] = done;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_HandleInstallSnapshotResponse(NodeId _Id, const InstallSnapshotResponse * _Response, google::protobuf::RpcController * _Controller)
{
    // 取出响应中的信息
    TermId other_term = _Response->term();

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::InstallSnapshotResponse;
    message.term = other_term;
    message.from = _Id;

    // 传入 Raft
    _Raft->step(message);
}

void RaftClerk::_HandleKVOperationRequest(const KVOperationRequest * _Request, google::protobuf::Closure * _Done)
{
    // 取出请求中的信息
    OperationType type = _Request->type();
    std::string uuid = _Request->meta().uuid();
    std::string key = _Request->key();
    std::string value = _Request->value();

    _Logger.debug("receive operation from client: " + uuid);

    // 构造上下文
    RaftMessage message;
    message.type = RaftMessage::MessageType::KVOperationRequest;
    message.uuid = uuid;
    message.key = key;
    message.value = value;

    switch (type) {
    case OperationType::PUT:
        message.op_type = RaftMessage::OperationType::PUT;
        break;
    case OperationType::UPDATE:
        message.op_type = RaftMessage::OperationType::UPDATE;
        break;
    case OperationType::DELETE:
        message.op_type = RaftMessage::OperationType::DELETE;
        break;
    case OperationType::GET:
        message.op_type = RaftMessage::OperationType::GET;
        break;
    default:
        break;
    }

    // 取出 Done 中的 sequence_id 并注册
    RaftRpcServerClosure * done = dynamic_cast<RaftRpcServerClosure *>(_Done);
    uint64_t sequence_id = done->sequenceId();
    message.seq = sequence_id;
    std::map<uint64_t, RaftRpcServerClosure *> & client_requests = _Pending_kv_requests[uuid];
    client_requests[sequence_id] = done;

    // 传入 Raft
    _Raft->step(message);
}

} // namespace WW
