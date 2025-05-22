#include "RaftNode.h"

namespace WW
{

RaftNode::RaftNode(NodeId _Id)
    : _Id(_Id)
    , _Term(0)
    , _Role(NodeRole::Follower)
    , _Logs()
    , _Voted_for(0)
    , _Vote_count(0)
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

int RaftNode::getVoteCount() const
{
    return _Vote_count;
}

LogIndex RaftNode::getLastLogIndex() const
{
    return _Logs.getLastIndex();
}

TermId RaftNode::getLastLogTerm() const
{
    return _Logs.getLastTerm();
}

bool RaftNode::match(LogIndex _Index, TermId _Term) const
{
    return _Logs.match(_Index, _Term);
}

void RaftNode::setTerm(TermId _Term)
{
    this->_Term = _Term;
}

void RaftNode::setRole(NodeRole _Role)
{
    this->_Role = _Role;
}

void RaftNode::setVotedFor(NodeId _Id)
{
    _Voted_for = _Id;
}

void RaftNode::setVoteCount(int _Vote_count)
{
    this->_Vote_count = _Vote_count;
}

} // namespace WW
