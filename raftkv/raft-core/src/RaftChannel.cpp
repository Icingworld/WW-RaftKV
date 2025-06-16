#include "RaftChannel.h"

#include <chrono>

namespace WW
{

void RaftChannel::push(const RaftMessage & _Message)
{
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _Queue.push(_Message);
    }
    _Cv.notify_one();
}

bool RaftChannel::pop(RaftMessage & _Message, int _Wait_ms)
{
    std::unique_lock<std::mutex> lock(_Mutex);
    if (_Queue.empty()) {
        if (_Wait_ms < 0) {
            return false;
        }

        if (!_Cv.wait_for(lock, std::chrono::milliseconds(_Wait_ms), [&] {
            return !_Queue.empty();
        })) {
            return false;
        }
    }

    _Message = std::move(_Queue.front());
    _Queue.pop();
    return true;
}

void RaftChannel::wakeup()
{
    _Cv.notify_all();
}

std::size_t RaftChannel::size()
{
    std::lock_guard<std::mutex> lock(_Mutex);
    return _Queue.size();
}

} // namespace WW
