#pragma once

#include <PageCache.h>

namespace WW
{

/**
 * @brief 中心缓存
 */
class CentralCache
{
private:
    std::array<SpanList, MAX_ARRAY_SIZE> _Spans;        // 页段链表数组
    std::mutex _Mutex;                                  // 中心缓存锁

private:
    CentralCache();

    CentralCache(const CentralCache &) = delete;

    CentralCache & operator=(const CentralCache &) = delete;

public:
    ~CentralCache() = default;

public:
    /**
     * @brief 获取中心缓存单例
     */
    static CentralCache & getCentralCache();

    /**
     * @brief 获取指定大小的空闲内存块
     * @param _Size 内存块大小
     * @param _Count 个数
     * @return 成功时返回`FreeObject *`，失败时返回`nullptr`
     */
    FreeObject * fetchRange(size_type _Size, size_type _Count);

    /**
     * @brief 将空闲内存块归还到中心缓存
     * @param _Size 内存块大小
     * @param _Free_object 空闲内存块链表
     */
    void returnRange(size_type _Size, FreeObject * _Free_object);

private:
    /**
     * @brief 获取一个空闲的页段
     * @param _Size 内存块大小
     * @return 成功时返回`Span *`，失败时返回`nullptr`
     */
    Span * _GetFreeSpan(size_type _Size);
};

} // namespace WW
