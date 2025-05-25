#pragma once

#include <functional>

#include <google/protobuf/service.h>

namespace WW
{

/**
 * @brief Closure 封装
*/
template <typename ResponseType>
class RaftRpcClosure : public google::protobuf::Closure
{
private:
    ResponseType * _Response;
    std::function<void(const ResponseType &)> _Callback;

public:
    RaftRpcClosure(ResponseType * _Response, std::function<void(const ResponseType &)> _Callback)
        : _Response(_Response)
        , _Callback(_Callback)
    {
    }

    ~RaftRpcClosure()
    {
        delete _Response;
    }

public:
    void Run() override
    {
        if (_Callback != nullptr) {
            _Callback(*_Response);
        }

        delete this;
    }
};

class RaftLambdaClosure : public google::protobuf::Closure
{
private:
    std::function<void()> _Func;

public:
    explicit RaftLambdaClosure(std::function<void()> func)
        : _Func(std::move(func))
    {
    }

public:
    void Run() override {
        _Func();

        delete this;
    }
};


} // namespace WW