#include "RaftRpcController.h"

namespace WW
{

RaftRpcController::RaftRpcController()
    : _Is_failed(false)
    , _Error_text("")
{
}

void RaftRpcController::Reset()
{
    _Is_failed = false;
    _Error_text.clear();
}

bool RaftRpcController::Failed() const
{
    return _Is_failed;
}

std::string RaftRpcController::ErrorText() const
{
    return _Error_text;
}

void RaftRpcController::SetFailed(const std::string & _Reason)
{
    _Is_failed = true;
    _Error_text = _Reason;
}

void RaftRpcController::StartCancel()
{
}

void RaftRpcController::NotifyOnCancel(google::protobuf::Closure * _Callback)
{
    (void)_Callback;
}

bool RaftRpcController::IsCanceled() const
{
    return _Is_canceled;
}

} // namespace WW
