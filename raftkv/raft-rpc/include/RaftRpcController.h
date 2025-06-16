#pragma once

#include <string>

#include <google/protobuf/service.h>

namespace WW
{

/**
 * @brief RaftRpcController
*/
class RaftRpcController : public google::protobuf::RpcController
{
private:
    bool _Is_failed;            // 是否失败
    bool _Is_canceled;          // 是否取消
    std::string _Error_text ;   // 失败原因

public:
    RaftRpcController();

    ~RaftRpcController() override = default;

public:
    void Reset() override;

    bool Failed() const override;

    std::string ErrorText() const override;

    void SetFailed(const std::string & _Reason) override;

    void StartCancel() override;

    void NotifyOnCancel(google::protobuf::Closure * _Callback) override;

    bool IsCanceled() const override;
};

} // namespace WW
