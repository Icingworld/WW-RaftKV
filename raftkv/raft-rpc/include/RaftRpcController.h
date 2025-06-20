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
    /**
     * @brief 重置
     */
    void Reset() override;

    /**
     * @brief 是否失败
    */
    bool Failed() const override;

    /**
     * @brief 错误原因
    */
    std::string ErrorText() const override;

    /**
     * @brief 设置失败
    */
    void SetFailed(const std::string & _Reason) override;

    /**
     * @brief 开始取消
    */
    void StartCancel() override;

    /**
     * @brief 取消时回调
    */
    void NotifyOnCancel(google::protobuf::Closure * _Callback) override;

    /**
     * @brief 是否取消
    */
    bool IsCanceled() const override;
};

} // namespace WW
