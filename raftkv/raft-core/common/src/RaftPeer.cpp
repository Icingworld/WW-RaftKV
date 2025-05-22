#include "RaftPeer.h"

namespace WW
{

RaftPeer::RaftPeer(NodeId _Id, const std::string & _Ip, const std::string & _Port)
    : _Id(_Id)
    , _Ip(_Ip)
    , _Port(_Port)
{
}

NodeId RaftPeer::getId() const
{
    return _Id;
}

const std::string & RaftPeer::getIp() const
{
    return _Ip;
}

const std::string & RaftPeer::getPort() const
{
    return _Port;
}

} // namespace WW
