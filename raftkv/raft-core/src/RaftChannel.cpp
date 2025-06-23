#include "RaftChannel.h"

namespace WW
{

std::unique_ptr<RaftMessage> RaftChannel::pop()
{
    std::unique_ptr<RaftMessage> result;
    _Queue.wait_dequeue(result);

    return result;
}

} // namespace WW
