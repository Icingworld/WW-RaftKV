#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>

#include <RaftMessage.h>

namespace WW
{

/**
 * @brief 消息通道
*/
class RaftChannel
{
private:
    std::queue<std::unique_ptr<RaftMessage>> _Queue;
    std::mutex _Mutex;
    std::condition_variable _Cv;

public:
    RaftChannel() = default;

    ~RaftChannel() = default;

public:
    /**
     * @brief 向通道添加消息
    */
    template <typename RaftMessageType>
    void push(RaftMessageType && _Message)
    {
        // 去掉 & 和 const
        using T = typename std::decay<RaftMessageType>::type;

        // 构造一个消息
        std::unique_ptr<RaftMessage> ptr = std::unique_ptr<T>(
            new T(std::forward<RaftMessageType>(_Message))
        );

        {
            std::lock_guard<std::mutex> lock(_Mutex);
            _Queue.push(std::move(ptr));
        }
        _Cv.notify_one();
    }

    /**
     * @brief 尝试获取通道中的消息
    */
    std::unique_ptr<RaftMessage> pop(int _Wait_ms);

    /**
     * @brief 唤醒所有等待线程
    */
    void wakeup();
};

} // namespace WW
