#pragma once

#include <functional>

#include <google/protobuf/service.h>

namespace WW
{

/**
 * @brief Closure 封装
*/
template <typename ResponseType>
class RaftClosure : public google::protobuf::Closure
{
private:
    ResponseType * _Response;
    std::function<void(const ResponseType &)> _Callback;

public:
    RaftClosure(ResponseType * _Response, std::function<void(const ResponseType &)> _Callback)
        : _Response(_Response)
        , _Callback(_Callback)
    {
    }

    ~RaftClosure() = default;

public:
    void Run() override
    {
        if (_Callback != nullptr) {
            _Callback(*_Response);
        }

        delete this;
    }
};

} // namespace WW
