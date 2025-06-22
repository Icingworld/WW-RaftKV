#include "RaftChannel.h"

#include <chrono>

namespace WW
{

std::unique_ptr<RaftMessage> RaftChannel::pop(int _Wait_ms)
{
    std::unique_ptr<RaftMessage> result;
    bool success = _Queue.wait_dequeue_timed(result, static_cast<int64_t>(_Wait_ms));
    if (success)
        return result;

    return nullptr;
}

} // namespace WW
