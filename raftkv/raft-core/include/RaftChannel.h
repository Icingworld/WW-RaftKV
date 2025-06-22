#pragma once

#include <memory>
#include <type_traits>

#include <RaftMessage.h>

#include <blockingconcurrentqueue.h>

namespace WW
{

/**
 * @brief 消息通道
*/
class RaftChannel
{
private:
    moodycamel::BlockingConcurrentQueue<std::unique_ptr<RaftMessage>> _Queue;  // 无锁队列

public:
    RaftChannel() = default;

    ~RaftChannel() = default;

public:
    /**
     * @brief 向通道添加消息
    */
    template <typename RaftMessageType>
    void push(RaftMessageType&& _Message)
    {
        // 去除引用和常量限定符
        using T = std::decay_t<RaftMessageType>;

        // 构造并入队
        std::unique_ptr<RaftMessage> ptr = std::make_unique<T>(std::forward<RaftMessageType>(_Message));
        _Queue.enqueue(std::move(ptr));
    }

    /**
     * @brief 获取消息，阻塞指定毫秒数
    */
    std::unique_ptr<RaftMessage> pop(int _Wait_ms);
};

} // namespace WW
