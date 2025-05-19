#pragma once

#include <mutex>

#include <FreeList.h>

namespace WW
{

/**
 * @brief 页段
 * @details 维护一大段连续的内存
 */
class Span
{
private:
    FreeList _Free_list;            // 空闲内存块
    size_type _Page_id;             // 页段号
    Span * _Prev;                   // 前一个页段
    Span * _Next;                   // 后一个页段
    size_type _Page_count;          // 页数
    size_type _Used;                // 已使用的内存块数

public:
    Span();

    ~Span() = default;

public:
    /**
     * @brief 获取页号
     */
    size_type page_id() const noexcept;

    /**
     * @brief 设置页号
     */
    void set_page_id(size_type _Page_id) noexcept;

    /**
     * @brief 获取页数
     */
    size_type page_count() const noexcept;

    /**
     * @brief 设置页数
     */
    void set_page_count(size_type _Page_count) noexcept;

    /**
     * @brief 获取上一个页段
     */
    Span * prev() const noexcept;

    /**
     * @brief 设置上一个页段
     */
    void set_prev(Span * _Prev) noexcept;

    /**
     * @brief 获取下一个页段
     */
    Span * next() const noexcept;

    /**
     * @brief 设置下一个页段
     */
    void set_next(Span * _Next) noexcept;

    /**
     * @brief 获取空闲内存块数量
     */
    size_type used() const noexcept;

    /**
     * @brief 设置空闲内存块数量
     */
    void set_used(size_type _Used) noexcept;

    /**
     * @brief 获取空闲内存块链表
     */
    FreeList * get_free_list() noexcept;

    /**
     * @brief 将内存地址转为页号
     */
    static size_type ptr_to_id(void * _Ptr) noexcept;

    /**
     * @brief 将内存地址转为页号
     */
    static void * id_to_ptr(size_type _Id) noexcept;
};

/**
 * @brief 页段链表迭代器
 */
class SpanListIterator
{
private:
    Span * _Span;      // 页段指针

public:
    explicit SpanListIterator(Span * _Span) noexcept;

    ~SpanListIterator() = default;

public:
    /**
     * @brief 迭代器是否相等
     */
    bool operator==(const SpanListIterator & _Other) const noexcept;

    /**
     * @brief 迭代器是否不相等
     */
    bool operator!=(const SpanListIterator & _Other) const noexcept;

    /**
     * @brief 解引用迭代器
     */
    Span & operator*() noexcept;

    /**
     * @brief 解引用迭代器
     */
    Span * operator->() noexcept;

    /**
     * @brief 向后移动
     */
    SpanListIterator & operator++() noexcept;

    /**
     * @brief 向后移动
     */
    SpanListIterator operator++(int) noexcept;

    /**
     * @brief 向前移动
     */
    SpanListIterator & operator--() noexcept;

    /**
     * @brief 向前移动
     */
    SpanListIterator operator--(int) noexcept;
};

/**
 * @brief 页段链表
 * @details 双向链表，持有一个互斥量，用于在中心缓存中的多线程访问
 */
class SpanList
{
public:
    using iterator = SpanListIterator;

private:
    Span _Head;                         // 虚拟头节点
    std::mutex _Mutex;                  // 链表递归锁

public:
    SpanList();

    ~SpanList() = default;

public:
    /**
     * @brief 获取第一个页段
     */
    Span & front() noexcept;

    /**
     * @brief 获取最后一个页段
     */
    Span & back() noexcept;

    /**
     * @brief 获取链表头部
     */
    iterator begin() noexcept;

    /**
     * @brief 获取链表尾部
     */
    iterator end() noexcept;

    /**
     * @brief 页段链表是否为空
     */
    bool empty() const noexcept;

    /**
     * @brief 将页段插入到头部
     */
    void push_front(Span * _Span) noexcept;

    /**
     * @brief 从头部删除页段
     */
    void pop_front() noexcept;

    /**
     * @brief 删除指定页段
     * @param span 要删除的页段
     */
    void erase(Span * _Span) noexcept;

    /**
     * @brief 给页段加锁
     */
    void lock() noexcept;

    /**
     * @brief 给页段解锁
     */
    void unlock() noexcept;
};

} // namespace WW
