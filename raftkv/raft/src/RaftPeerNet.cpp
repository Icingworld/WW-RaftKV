#include "RaftPeerNet.h"

namespace WW
{

RaftPeerNet::RaftPeerNet(NodeId _Id, const std::string & _Ip, const std::string & _Port)
    : _Id(_Id)
    , _Ip(_Ip)
    , _Port(_Port)
{
}

NodeId RaftPeerNet::getId() const
{
    return _Id;
}

const std::string & RaftPeerNet::getIp() const
{
    return _Ip;
}

const std::string & RaftPeerNet::getPort() const
{
    return _Port;
}

} // namespace WW
