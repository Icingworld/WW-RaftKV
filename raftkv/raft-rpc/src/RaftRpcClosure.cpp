#include "RaftRpcClosure.h"

namespace WW
{

RaftRpcServerClosure::RaftRpcServerClosure(SequenceType _Sequence_id, google::protobuf::Message * _Response, ResponseCallback _Callback)
    : _Sequence_id(_Sequence_id)
    , _Response(_Response)
    , _Callback(std::move(_Callback))
{
}

RaftRpcServerClosure::~RaftRpcServerClosure()
{
    delete _Response;
}

void RaftRpcServerClosure::Run()
{
    _Callback();

    delete this;
}

google::protobuf::Message * RaftRpcServerClosure::response()
{
    return _Response;
}

SequenceType RaftRpcServerClosure::sequenceId() const
{
    return _Sequence_id;
}

} // namespace WW
