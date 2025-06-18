#include "RaftRpcClosure.h"

namespace WW
{

RaftRpcServerClosure::RaftRpcServerClosure(SequenceType _Sequence_id,
                                        std::unique_ptr<google::protobuf::RpcController> _Controller,
                                        std::unique_ptr<google::protobuf::Message> _Request,
                                        std::unique_ptr<google::protobuf::Message> _Response,
                                        ResponseCallback && _Callback)
    : _Sequence_id(_Sequence_id)
    , _Controller(std::move(_Controller))
    , _Request(std::move(_Request))
    , _Response(std::move(_Response))
    , _Callback(std::move(_Callback))
{
}

void RaftRpcServerClosure::Run()
{
    _Callback();

    delete this;
}

google::protobuf::Message * RaftRpcServerClosure::response()
{
    return _Response.get();
}

SequenceType RaftRpcServerClosure::sequenceId() const
{
    return _Sequence_id;
}

} // namespace WW
