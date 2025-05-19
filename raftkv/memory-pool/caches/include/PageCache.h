#pragma once

#include <array>
#include <vector>
#include <unordered_map>
#include <map>

#include <SpanList.h>

namespace WW
{

/**
 * @brief 页缓存
 */
class PageCache
{
private:
    std::array<SpanList, MAX_PAGE_NUM> _Spans;              // 页段链表数组
    std::unordered_map<size_type, Span *> _Free_span_map;   // 页号到空闲页段指针的映射
    std::map<size_type, Span *> _Busy_span_map;             // 页号到繁忙页段指针的映射
    std::vector<void *> _Align_pointers;                    // 对齐指针数组
    std::mutex _Mutex;                                      // 页缓存锁

private:
    PageCache();

    PageCache(const PageCache &) = delete;

    PageCache & operator=(const PageCache &) = delete;

public:
    ~PageCache();

public:
    /**
     * @brief 获取页缓存单例
     */
    static PageCache & getPageCache();

    /**
     * @brief 获取指定大小的页段
     * @param _Pages 页数
     * @return 成功时返回`Span *`，失败时返回`nullptr`
     */
    Span * fetchSpan(size_type _Pages);

    /**
     * @brief 将页段归还到页缓存
     * @param _Span 页段
     */
    void returnSpan(Span * _Span);

    /**
     * @brief 通过内存块指针找到对应页段
     * @param _Ptr 内存块指针
     * @return 成功时返回`Span *`，失败时返回`nullptr`
     */
    Span * objectToSpan(void * _Ptr) noexcept;

private:
    /**
     * @brief 从系统内存中获取指定大小的内存
     * @param _Pages 页数
     * @return 成功时返回`void *`，失败时返回`nullptr`
     */
    void * _FetchFromSystem(size_type _Pages) const noexcept;
};

} // namespace WW
