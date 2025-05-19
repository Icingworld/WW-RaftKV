#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#include <LoggerBuffer.h>

namespace WW
{

/**
 * @brief 异步日志管理线程
 */
class AsyncWorker
{
private:
    LoggerBuffer producter_buffer;          // 生产者日志缓冲区
    LoggerBuffer consumer_buffer;           // 消费者日志缓冲区

    std::function<void(const char *, std::size_t)> callback;    // 回调函数

    std::atomic<bool> running;              // 是否运行中
    std::mutex mutex;                       // 互斥锁
    std::condition_variable producter_cv;   // 生产者条件变量
    std::condition_variable consumer_cv;    // 消费者条件变量
    std::thread thread;                     // 线程

public:
    AsyncWorker(std::function<void(const char *, std::size_t)> callback);

    explicit AsyncWorker(std::size_t size, std::function<void(const char *, std::size_t)> callback);

    ~AsyncWorker();

public:
    /**
     * @brief 写一条日志
     */
    void push(const char * data, std::size_t size);

    /**
     * @brief 关闭线程
     */
    void stop();

private:
    /**
     * @brief 工作线程
     */
    void workingThread();
};

} // namespace WW
