#include "RaftMessage.h"

namespace WW
{

RaftMessage::RaftMessage(MessageType type)
    : type(type)
{
}

RaftRequestVoteRequestMessage::RaftRequestVoteRequestMessage()
    : RaftMessage(MessageType::RequestVoteRequest)
{
}

RaftRequestVoteResponseMessage::RaftRequestVoteResponseMessage()
    : RaftMessage(MessageType::RequestVoteResponse)
{
}

RaftAppendEntriesRequestMessage::RaftAppendEntriesRequestMessage()
    : RaftMessage(MessageType::AppendEntriesRequest)
{
}

RaftAppendEntriesResponseMessage::RaftAppendEntriesResponseMessage()
    : RaftMessage(MessageType::AppendEntriesResponse)
{
}

RaftInstallSnapshotRequestMessage::RaftInstallSnapshotRequestMessage()
    : RaftMessage(MessageType::InstallSnapshotRequest)
{
}

RaftInstallSnapshotResponseMessage::RaftInstallSnapshotResponseMessage()
    : RaftMessage(MessageType::InstallSnapshotResponse)
{
}

KVOperationRequestMessage::KVOperationRequestMessage()
    : RaftMessage(MessageType::KVOperationRequest)
{
}

KVOperationResponseMessage::KVOperationResponseMessage()
    : RaftMessage(MessageType::KVOPerationResponse)
{
}

ApplyCommitLogsRequestMessage::ApplyCommitLogsRequestMessage()
    : RaftMessage(MessageType::ApplyCommitLogsRequest)
{
}

ApplyCommitLogsResponseMessage::ApplyCommitLogsResponseMessage()
    : RaftMessage(MessageType::ApplyCommitLogsResponse)
{
}

GenerateSnapshotRequestMessage::GenerateSnapshotRequestMessage()
    : RaftMessage(MessageType::GenerateSnapshotRequest)
{
}

GenerateSnapshotResponseMessage::GenerateSnapshotResponseMessage()
    : RaftMessage(MessageType::GenerateSnapshotResponse)
{
}

ApplySnapshotRequestMessage::ApplySnapshotRequestMessage()
    : RaftMessage(MessageType::ApplySnapshotRequest)
{
}

ApplySnapshotResponseMessage::ApplySnapshotResponseMessage()
    : RaftMessage(MessageType::ApplySnapshotResponse)
{
}

ShutdownMessage::ShutdownMessage()
    : RaftMessage(MessageType::Shutdown)
{
}

} // namespace WW
