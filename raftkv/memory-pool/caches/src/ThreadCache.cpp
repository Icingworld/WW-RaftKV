#include "ThreadCache.h"

#include <Size.h>

namespace WW
{

ThreadCache::ThreadCache()
    : _Free_lists()
{
}

ThreadCache::~ThreadCache()
{
    // 归还所有内存块
    for (size_type _I = 0; _I < _Free_lists.size(); ++_I) {
        if (!_Free_lists[_I].empty()) {
            _ReturnToCentralCache(_I, _Free_lists[_I].size());
        }
    }
}

ThreadCache & ThreadCache::getThreadCache()
{
    // 线程局部存储的单例
    static thread_local ThreadCache _ThreadCache;
    return _ThreadCache;
}

void * ThreadCache::allocate(size_type _Size) noexcept
{
    if (_Size == 0) {
        return nullptr;
    }

    if (_Size > MAX_MEMORY_SIZE) {
        // 超出管理范围，直接从堆获取
        return ::operator new(_Size, std::nothrow);
    }

    // 获取对齐后的大小
    size_type _Round_size = Size::roundUp(_Size);
    // 找到所在的索引
    size_type _Index = Size::sizeToIndex(_Round_size);

    if (_Free_lists[_Index].empty()) {
        // 没有这种内存块，需要申请
        _FetchFromCentralCache(_Round_size);
    }

    // 有这种内存块，取一个出来
    FreeObject * _Obj = _Free_lists[_Index].front();
    _Free_lists[_Index].pop_front();
    return reinterpret_cast<void *>(_Obj);
}

void ThreadCache::deallocate(void * _Ptr, size_type _size) noexcept
{
    if (_size == 0) {
        return;
    }

    if (_size > MAX_MEMORY_SIZE) {
        // 从系统释放
        ::operator delete(_Ptr, std::nothrow);
        return;
    }

    // 获取对齐后的大小
    size_type _Round_size = Size::roundUp(_size);
    // 找到所在的索引
    size_type _Index = Size::sizeToIndex(_Round_size);
    // 把内存插入自由表
    FreeObject * _Obj = reinterpret_cast<FreeObject *>(_Ptr);
    _Free_lists[_Index].push_front(_Obj);

    // 检查是否需要归还给中心缓存
    if (_ShouldReturn(_Index)) {
        _ReturnToCentralCache(_Index, _Free_lists[_Index].max_size());
    }
}

bool ThreadCache::_ShouldReturn(size_type _Index) const noexcept
{
    // 超过一次申请的最大数量的两倍，归还一半
    if (_Free_lists[_Index].size() >= _Free_lists[_Index].max_size() * 2) {
        return true;
    }

    return false;
}

void ThreadCache::_FetchFromCentralCache(size_type _Size) noexcept
{
    // 每次申请按照最大数量申请，并且提升最大数量
    size_type _Index = Size::sizeToIndex(_Size);
    size_type _Count = _Free_lists[_Index].max_size();
    if (_Count > MAX_BLOCK_NUM) {
        _Count = MAX_BLOCK_NUM;
    }

    FreeObject * _Obj = CentralCache::getCentralCache().fetchRange(_Size, _Count);
    FreeObject * _Cur = _Obj;
    
    while (_Cur != nullptr) {
        FreeObject * _Next = _Cur->next();
        _Free_lists[_Index].push_front(_Cur);
        _Cur = _Next;
    }

    // 提升最大数量
    _Free_lists[_Index].set_max_size(_Count + 1);
}

void ThreadCache::_ReturnToCentralCache(size_type _Index, size_type _Nums) noexcept
{
    // 取出nums个内存块组成链表
    FreeObject * _Head = nullptr;

    for (size_type _I = 0; _I < _Nums; ++_I) {
        FreeObject * _Obj = _Free_lists[_Index].front();
        _Free_lists[_Index].pop_front();
        _Obj->set_next(_Head);
        _Head = _Obj;
    }

    CentralCache::getCentralCache().returnRange(Size::indexToSize(_Index), _Head);
}

} // namespace WW
