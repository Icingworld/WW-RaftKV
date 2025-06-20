#pragma once

#include <ThreadCache.h>

namespace WW
{

/**
 * @brief 基于线程池的分配器
*/
template <typename _Type>
class MemoryPoolAllocator
{
    template <typename _U>
    friend class MemoryPoolAllocator;   // 允许不同类型的分配器互相访问缓存

public:
    using value_type = _Type;
    using pointer = _Type *;
    using const_pointer = const _Type *;
    using reference = _Type &;
    using const_reference = const _Type &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template <typename _U>
    class rebind
    {
    public:
        using other = MemoryPoolAllocator<_U>;
    };

private:
    ThreadCache & _Thread_cache;        // 线程缓存

public:
    MemoryPoolAllocator()
        : _Thread_cache(ThreadCache::getThreadCache())
    {
    }

    MemoryPoolAllocator(const MemoryPoolAllocator & other)
        : _Thread_cache(other._Thread_cache)
    {
    }
    
    template <typename _U>
    MemoryPoolAllocator(const MemoryPoolAllocator<_U> & other) 
        : _Thread_cache(other._Thread_cache)
    { // template cannot be default
    };

    ~MemoryPoolAllocator() = default;

public:
    /**
     * @brief 分配 _N 个元素的内存
     * @param _N 元素个数
     * @param _Hint 会在 hint 附近分配内存，忽略
     * @return `pointer` 内存指针
     * @exception `std::bad_array_new_length` 超出最大尺寸
     * @exception `std::bad_alloc` 内存分配失败
     */
    pointer allocate(size_type _N, const void * _Hint = nullptr)
    {
        (void)_Hint;
        if (_N > max_size())   // 超出最大尺寸
            throw std::bad_array_new_length();

        if (_N == 0)
            return nullptr;

        return static_cast<pointer>(_Thread_cache.allocate(_N * sizeof(_Type)));
    }

    /**
     * @brief 释放由 allocate 分配的内存
     * @param _Ptr 要释放的内存指针
     * @param _N 元素个数
     */
    void deallocate(pointer _Ptr, size_type _N)
    {
        if (_Ptr == nullptr)
            return;
        
        _Thread_cache.deallocate(_Ptr, _N * sizeof(_Type));
    }

    /**
     * @brief 构造对象
     * @param _Ptr 要构造的内存指针
     * @param args 构造函数参数包
     */
    template <typename _U, typename... _Args>
    void construct(_U * _Ptr, _Args&&... args)
    {
        ::new(_Ptr) _U(std::forward<_Args>(args)...);
    }

    /**
     * @brief 销毁对象
     * @param _Ptr 要销毁的内存指针
     */
    template <typename _U>
    void destroy(_U * _Ptr)
    {
        _Ptr->~_U();
    }

    /**
     * @brief 最大大小
     */
    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

    /**
     * @brief 获取地址
     */
    pointer address(reference _R) const noexcept
    {
        return &_R;
    }

    /**
     * @brief 获取地址
     */
    const_pointer address(const_reference _R) const noexcept
    {
        return &_R;
    }

    bool operator==(const MemoryPoolAllocator & other) const noexcept
    {
        return true;
    }

    bool operator!=(const MemoryPoolAllocator & other) const noexcept
    {
        return false;
    }
};

} // namespace WW
