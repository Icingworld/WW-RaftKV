#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

#include <RaftMessage.h>

namespace WW
{

/**
 * @brief 消息通道
*/
class RaftChannel
{
private:
    std::queue<RaftMessage> _Queue;
    std::mutex _Mutex;
    std::condition_variable _Cv;

public:
    RaftChannel() = default;

    ~RaftChannel() = default;

public:
    /**
     * @brief 向通道添加消息
    */
    void push(const RaftMessage & _Message);

    /**
     * @brief 尝试获取通道中的消息
    */
    bool pop(RaftMessage & _Message, int _Wait_ms);

    /**
     * @brief 唤醒所有等待线程
    */
    void wakeup();

    std::size_t size();
};

} // namespace WW
