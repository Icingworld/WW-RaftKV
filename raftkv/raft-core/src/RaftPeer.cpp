#include "RaftPeer.h"

namespace WW
{

RaftPeer::RaftPeer(NodeId _Id)
    : _Id(_Id)
    , _Next_index(-1)
    , _Match_index(-1)
{
}

NodeId RaftPeer::getId() const
{
    return _Id;
}

LogIndex RaftPeer::getNextIndex() const
{
    return _Next_index;
}

LogIndex RaftPeer::getMatchIndex() const
{
    return _Match_index;
}

void RaftPeer::setNextIndex(LogIndex _Next_index)
{
    this->_Next_index = _Next_index;
}

void RaftPeer::setMatchIndex(LogIndex _Match_index)
{
    this->_Match_index = _Match_index;
}

} // namespace WW
