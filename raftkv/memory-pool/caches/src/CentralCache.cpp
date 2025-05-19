#include "CentralCache.h"

#include <algorithm>

#include <Size.h>

namespace WW
{

CentralCache::CentralCache()
    : _Spans()
{
}

CentralCache & CentralCache::getCentralCache()
{
    static CentralCache _CentralCache;
    return _CentralCache;
}

FreeObject * CentralCache::fetchRange(size_type _Size, size_type _Count)
{
    size_type _Index = Size::size_to_index(_Size);

    // 锁住index对应链表
    _Spans[_Index].lock();

    // 获取一个非空的空闲页
    Span * _Span = _GetFreeSpan(_Size);
    if (_Span == nullptr) {
        _Spans[_Index].unlock();
        
        return nullptr;
    }

    FreeObject * _Head = nullptr;
    for (size_type _I = 0; _I < _Count; ++_I) {
        // 取出一个内存块，并且插入到新链表的头部
        FreeObject * _New_object = _Span->get_free_list()->front();
        _Span->get_free_list()->pop_front();
        _New_object->set_next(_Head);
        _Head = _New_object;
        _Span->set_used(_Span->used() + 1);

        if (_Span->get_free_list()->empty()) {
            // 已经空了，直接返回这么多内存块
            break;
        }
    }

    _Spans[_Index].unlock();

    return _Head;
}

void CentralCache::returnRange(size_type size, FreeObject * _Free_object)
{
    size_type _Index = Size::size_to_index(size);

    // 锁住index对应链表
    _Spans[_Index].lock();

    // 依次归还空闲内存块
    while (_Free_object != nullptr) {
        FreeObject * _Next = _Free_object->next();

        // 查找该内存块属于哪个页段
        void * _Ptr = reinterpret_cast<void *>(_Free_object);

        _Spans[_Index].unlock();
        Span * _Span = PageCache::getPageCache().objectToSpan(_Ptr);
        _Spans[_Index].lock();

        // 没找到则跳过，这种情况不应该出现
        if (_Span == nullptr) {
            _Free_object = _Next;
            continue;
        }

        // 将内存块加入到该页段的空闲链表中
        _Span->get_free_list()->push_front(_Free_object);
        // 同步页段使用数量
        _Span->set_used(_Span->used() - 1);

        if (_Span->used() == 0) {
            // 已经使用完毕，可以从链表中删除
            _Spans[_Index].erase(_Span);
            _Span->get_free_list()->clear();

            // 归还页缓存
            _Spans[_Index].unlock();
            PageCache::getPageCache().returnSpan(_Span);
            _Spans[_Index].lock();
        }

        // 向后移动
        _Free_object = _Next;
    }

    _Spans[_Index].unlock();
}

Span * CentralCache::_GetFreeSpan(size_type _Size)
{
    size_type _Index = Size::size_to_index(_Size);

    for (auto _It = _Spans[_Index].begin(); _It != _Spans[_Index].end(); ++_It) {
        // 遍历链表，查看是否有已经切过的页
        if (!_It->get_free_list()->empty()) {
            return &*_It;
        }
    }

    _Spans[_Index].unlock();

    // 没找到空闲的页段，需要向页缓存申请
    // 计算出应该申请多少页的页段

    // 尝试申请最大数量的内存块，计算申请的总内存大小
    size_type _Total_size = _Size * MAX_BLOCK_NUM;
    // 初步计算需要多少页
    size_type _Page_count = _Total_size / PAGE_SIZE;
    if (_Total_size % PAGE_SIZE != 0) {
        _Page_count += 1;
    }

    // 不能超过最大页数
    if (_Page_count > MAX_PAGE_NUM) {
        _Page_count = MAX_PAGE_NUM;
    }

    // 申请页段
    Span * _Span = PageCache::getPageCache().fetchSpan(_Page_count);
    if (_Span == nullptr) {
        return nullptr;
    }
    
    // 将页段切成size大小的内存块，挂载到freelist上
    // 计算出内存的起始地址
    void * _Ptr = Span::id_to_ptr(_Span->page_id());

    // 计算每个内存块的大小并将其挂到 freelist 上
    size_type _Block_num = _Span->page_count() * PAGE_SIZE / _Size;
    for (size_type _I = 0; _I < _Block_num; ++_I) {
        // 偏移每次一个内存块的大小
        void * _Block_ptr = static_cast<char *>(_Ptr) + _I * _Size;
        // 创建 FreeObject 来管理内存块
        FreeObject * _Free_obj = reinterpret_cast<FreeObject *>(_Block_ptr);
        // 将此内存块挂到 freelist 上
        _Span->get_free_list()->push_front(_Free_obj);
    }

    // 将页段挂到链表上
    _Spans[_Index].lock();
    _Spans[_Index].push_front(_Span);

    return _Span;
}

} // namespace WW
