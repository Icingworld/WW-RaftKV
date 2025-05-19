#pragma once

#include <Common.h>

namespace WW
{

/**
 * @brief 空闲内存块
 */
class FreeObject
{
private:
    FreeObject * _Next;  // 下一个空闲内存块

public:
    FreeObject();

    explicit FreeObject(FreeObject * _Next);

    ~FreeObject() = default;

public:
    /**
     * @brief 获取下一个空闲内存块
     */
    FreeObject * next() const noexcept;

    /**
     * @brief 设置下一个空闲内存块
     */
    void set_next(FreeObject * _Next) noexcept;
};

/**
 * @brief 空闲内存块链表迭代器
 */
class FreeListIterator
{
private:
    FreeObject * _Free_object;       // 空闲内存块指针

public:
    explicit FreeListIterator(FreeObject * _Free_object) noexcept;

    ~FreeListIterator() = default;

public:
    /**
     * @brief 迭代器是否相等
     */
    bool operator==(const FreeListIterator & _Other) const noexcept;

    /**
     * @brief 迭代器是否不相等
     */
    bool operator!=(const FreeListIterator & _Other) const noexcept;

    /**
     * @brief 解引用迭代器
     * @details 对于内存块，解引用直接返回内存块地址
     */
    FreeObject * operator*() noexcept;

    /**
     * @brief 解引用迭代器
     */
    FreeObject * operator->() noexcept;

    /**
     * @brief 向后移动
     */
    FreeListIterator & operator++() noexcept;

    /**
     * @brief 向后移动
     */
    FreeListIterator operator++(int) noexcept;
};

/**
 * @brief 空闲内存块链表
 * @details 单向链表
 */
class FreeList
{
public:
    using iterator = FreeListIterator;

private:
    FreeObject _Head;           // 虚拟头节点
    size_type _Size;            // 空闲内存块数量
    size_type _Max_size;        // 最大数量

public:
    FreeList();

    ~FreeList() = default;

public:
    /**
     * @brief 获取链表头部元素
     */
    FreeObject * front() noexcept;

    /**
     * @brief 将空闲内存块插入到链表头部
     */
    void push_front(FreeObject * _Free_object);

    /**
     * @brief 从链表头部移除内存块
     */
    void pop_front();

    /**
     * @brief 获取链表头部
     */
    iterator begin() noexcept;

    /**
     * @brief 获取链表尾部
     */
    iterator end() noexcept;

    /**
     * @brief 链表是否为空
     */
    bool empty() const noexcept;

    /**
     * @brief 获取空闲内存块数量
     */
    size_type size() const noexcept;

    /**
     * @brief 获取最大大小
     */
    size_type max_size() const noexcept;

    /**
     * @brief 设置最大大小
     */
    void set_max_size(size_type _Max_size) noexcept;

    /**
     * @brief 清空链表
     */
    void clear() noexcept;
};

} // namespace WW
