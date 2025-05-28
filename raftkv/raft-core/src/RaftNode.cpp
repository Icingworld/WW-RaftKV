#include "RaftNode.h"

namespace WW
{

RaftNode::RaftNode(NodeId _Id)
    : _Id(_Id)
    , _Term(0)
    , _Role(NodeRole::Follower)
    , _Logs()
    , _Voted_for(-1)
    , _Leader_id(-1)
    , _Last_commit_index(0)
    , _Last_applied_index(0)
{
}

NodeId RaftNode::getId() const
{
    return _Id;
}

TermId RaftNode::getTerm() const
{
    return _Term;
}

RaftNode::NodeRole RaftNode::getRole() const
{
    return _Role;
}

NodeId RaftNode::getVotedFor() const
{
    return _Voted_for;
}

NodeId RaftNode::getLeaderId() const
{
    return _Leader_id;
}

LogIndex RaftNode::getLastCommitIndex() const
{
    return _Last_commit_index;
}

LogIndex RaftNode::getLastAppliedIndex() const
{
    return _Last_applied_index;
}

LogIndex RaftNode::getLastIndex() const
{
    return _Logs.getLastIndex();
}

TermId RaftNode::getLastTerm() const
{
    return _Logs.getLastTerm();
}

const RaftLogEntry & RaftNode::getLog(LogIndex _Index) const
{
    return _Logs.at(_Index);
}

TermId RaftNode::getTerm(LogIndex _Index) const
{
    return _Logs.getTerm(_Index);
}

bool RaftNode::isFollower() const
{
    return _Role == NodeRole::Follower;
}

bool RaftNode::isCandidate() const
{
    return _Role == NodeRole::Candidate;
}

bool RaftNode::isLeader() const
{
    return _Role == NodeRole::Leader;
}

bool RaftNode::match(LogIndex _Index, TermId _Term) const
{
    return _Logs.match(_Index, _Term);
}

void RaftNode::append(const RaftLogEntry & _Log_entry)
{
    _Logs.append(_Log_entry);
}

void RaftNode::truncate(LogIndex _Truncate_index)
{
    _Logs.truncate(_Truncate_index);
}

std::vector<RaftLogEntry> RaftNode::getLogFrom(LogIndex _Index)
{
    return _Logs.getLogFrom(_Index);
}

void RaftNode::setTerm(TermId _Term)
{
    this->_Term = _Term;
}

void RaftNode::setRole(NodeRole _Role)
{
    this->_Role = _Role;
}

void RaftNode::switchToFollower()
{
    _Role = NodeRole::Follower;
}

void RaftNode::switchToCandidate()
{
    _Role = NodeRole::Candidate;
}

void RaftNode::switchToLeader()
{
    _Role = NodeRole::Leader;
}

void RaftNode::setVotedFor(NodeId _Id)
{
    _Voted_for = _Id;
}

void RaftNode::setLeaderId(NodeId _Id)
{
    _Leader_id = _Id;
}

void RaftNode::setLastCommitIndex(LogIndex _Last_commit_index)
{
    this->_Last_commit_index = _Last_commit_index;
}

void RaftNode::setLastAppliedIndex(LogIndex _Last_applied_index)
{
    this->_Last_applied_index = _Last_applied_index;
}

} // namespace WW
