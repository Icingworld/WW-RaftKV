#include "RaftChannel.h"

#include <chrono>

namespace WW
{

std::unique_ptr<RaftMessage> RaftChannel::pop(int _Wait_ms)
{
    std::unique_lock<std::mutex> lock(_Mutex);
    if (_Queue.empty()) {
        if (_Wait_ms < 0) {
            return nullptr;
        }

        if (!_Cv.wait_for(lock, std::chrono::milliseconds(_Wait_ms), [&] {
            return !_Queue.empty();
        })) {
            return nullptr;
        }
    }

    // 取出一个消息
    std::unique_ptr<RaftMessage> ptr = std::move(_Queue.front());
    _Queue.pop();

    return std::move(ptr);
}

void RaftChannel::wakeup()
{
    _Cv.notify_all();
}

} // namespace WW
