#include "AsyncWorker.h"

#include <DefaultFileSink.h>

namespace WW
{

AsyncWorker::AsyncWorker(std::function<void(const char *, std::size_t)> callback)
    : producter_buffer()
    , consumer_buffer()
    , callback(callback)
    , running(true)
{
    thread = std::thread(&AsyncWorker::workingThread, this);
}

AsyncWorker::AsyncWorker(std::size_t size, std::function<void(const char *, std::size_t)> callback)
    : producter_buffer(size)
    , consumer_buffer(size)
    , running(true)
    , callback(callback)
{
    thread = std::thread(&AsyncWorker::workingThread, this);
}

AsyncWorker::~AsyncWorker()
{
    if (running.load()) {
        stop();
    }
}

void AsyncWorker::push(const char * data, std::size_t size)
{
    std::unique_lock<std::mutex> lock(mutex);
    producter_cv.wait(lock, [&]() {
       return producter_buffer.available(size); 
    });

    producter_buffer.push(data, size);
    consumer_cv.notify_one();
}

void AsyncWorker::stop()
{
    running.store(false);
    consumer_cv.notify_all();
    thread.join();
}

void AsyncWorker::workingThread()
{
    while (running.load() || !producter_buffer.empty()) {
        std::unique_lock<std::mutex> lock(mutex);
        consumer_cv.wait(lock, [&]() {
            return !running.load() || !producter_buffer.empty();
        });

        producter_buffer.swap(consumer_buffer);
        producter_cv.notify_one();
        lock.unlock();

        // consume data
        char * data = nullptr;
        std::size_t size = 0;
        consumer_buffer.read(data, size);
        callback(data, size);

        consumer_buffer.reset();
    }
}

} // namespace WW
