#pragma once

#include <CentralCache.h>

namespace WW
{

/**
 * @brief 线程缓存
 */
class ThreadCache
{
private:
    std::array<FreeList, MAX_ARRAY_SIZE> _Free_lists;       // 自由表数组

private:
    ThreadCache();

    ThreadCache(const ThreadCache &) = delete;

    ThreadCache & operator=(const ThreadCache &) = delete;

public:
    ~ThreadCache();

public:
    /**
     * @brief 获取线程缓存单例
     */
    static ThreadCache & getThreadCache();

    /**
     * @brief 申请内存
     * @param _Size 内存大小
     * @return 成功返回`void *`，失败返回`nullptr`
     */
    void * allocate(size_type _Size) noexcept;

    /**
     * @brief 回收内存
     * @param _Ptr 内存指针
     * @param _Size 内存大小
     */
    void deallocate(void * _Ptr, size_type _Size) noexcept;

private:
    /**
     * @brief 判断是否需要归还给中心缓存
     * @param _Index 索引
     */
    bool _ShouldReturn(size_type _Index)  const noexcept;

    /**
     * @brief 从中心缓存获取一批内存块
     * @param _Size 申请的内存块大小
     */
    void _FetchFromCentralCache(size_type _Size) noexcept;

    /**
     * @brief 将一批内存块还给中心缓存
     * @param _Index 归还内存所在的索引
     * @param _Nums 归还的内存数量
     */
    void _ReturnToCentralCache(size_type _Index, size_type _Nums) noexcept;
};
    
} // namespace WW
