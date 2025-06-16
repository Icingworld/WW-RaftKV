#include "RaftPeerNet.h"

namespace WW
{

RaftPeerNet::RaftPeerNet(NodeId _Id, const std::string & _Ip, const std::string & _Port, const std::string & _KV_port)
    : _Id(_Id)
    , _Ip(_Ip)
    , _Port(_Port)
    , _KV_port(_KV_port)
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

const std::string & RaftPeerNet::getKVPort() const
{
    return _KV_port;
}

} // namespace WW
