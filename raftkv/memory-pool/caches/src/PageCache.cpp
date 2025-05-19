#include "PageCache.h"

#include <Platform.h>
#include <cassert>

namespace WW
{

PageCache::PageCache()
    : _Spans()
    , _Free_span_map()
    , _Busy_span_map()
    , _Align_pointers()
    , _Mutex()
{
}

PageCache::~PageCache()
{
    std::lock_guard<std::mutex> _Lock(_Mutex);
    
    // 释放所有页段
    for (size_type _I = 0; _I < MAX_PAGE_NUM; ++_I) {
        while (!_Spans[_I].empty()) {
            Span & _Span = _Spans[_I].front();
            _Spans[_I].pop_front();
            // 销毁页段
            delete &_Span;
        }
    }

    // 释放所有对齐指针
    for (void * _Ptr : _Align_pointers) {
        Platform::aligned_free(_Ptr);
    }
}

PageCache & PageCache::getPageCache()
{
    static PageCache _Instance;
    return _Instance;
}

Span * PageCache::fetchSpan(size_type _Pages)
{
    std::lock_guard<std::mutex> _Lock(_Mutex);

    // 如果有空闲页段，直接从链表中直接获取页段
    if (!_Spans[_Pages - 1].empty()) {
        // 从链表中取出页段
        Span & _Span = _Spans[_Pages - 1].front();
        _Spans[_Pages - 1].pop_front();

        // 从空闲映射表中移除页段
        _Free_span_map.erase(_Span.page_id());
        _Free_span_map.erase(_Span.page_id() + _Span.page_count() - 1);

        // 将页段插入到繁忙映射表中
        _Busy_span_map[_Span.page_id()] = &_Span;
        _Busy_span_map[_Span.page_id() + _Span.page_count() - 1] = &_Span;

        return &_Span;
    }

    // 没有正好这么大的页段，尝试从更大块内存中切出页段
    for (size_type _I = _Pages; _I < MAX_PAGE_NUM; ++_I) {
        if (!_Spans[_I].empty()) {
            // 取出页段，该页段页数为i + 1
            Span & _Bigger_span = _Spans[_I].front();
            _Spans[_I].pop_front();

            // 切分i + 1页的页段，切成i = (i + 1 - pages) + pages的两个页段

            // 新建一个split_span用于储存后面长pages页的页段
            Span * _Split_span = new Span();
            _Split_span->set_page_id(_Bigger_span.page_id() + _I + 1 - _Pages);
            _Split_span->set_page_count(_Pages);

            // bigger_span用于储存前面长i + 1 - pages的页段，直接修改页数
            _Bigger_span.set_page_count(_I + 1 - _Pages);

            // 前面的页段插入到对应链表中
            _Spans[_Bigger_span.page_count() - 1].push_front(&_Bigger_span);

            // 修改映射表
            // 解除原bigger_span的尾页号映射
            _Free_span_map.erase(_Bigger_span.page_id() + _Bigger_span.page_count() + _Split_span->page_count() - 1);
            // 添加新bigger_span的尾页号映射
            _Free_span_map[_Bigger_span.page_id() + _Bigger_span.page_count() - 1] = &_Bigger_span;

            // split_span页号插入繁忙映射表
            _Busy_span_map[_Split_span->page_id()] = _Split_span;
            _Busy_span_map[_Split_span->page_id() + _Split_span->page_count() - 1] = _Split_span;

            return _Split_span;
        }
    }

    // 没找到更大的页段，直接申请一个最大的页段，然后按照上面的流程重新获取页段
    void * _Ptr = _FetchFromSystem(MAX_PAGE_NUM);
    if (_Ptr == nullptr) {
        return nullptr;
    }

    // 记录该对齐指针
    _Align_pointers.emplace_back(_Ptr);

    Span * _Max_span = new Span();

    if (_Pages == MAX_PAGE_NUM) {
        // 恰好需要最大页数
        _Max_span->set_page_id(Span::ptr_to_id(_Ptr));
        _Max_span->set_page_count(MAX_PAGE_NUM);

        // 建立繁忙表映射
        _Busy_span_map[_Max_span->page_id()] = _Max_span;
        _Busy_span_map[_Max_span->page_id() + _Max_span->page_count() - 1] = _Max_span;

        return _Max_span;
    }
    
    // max_span用来储存MAX_PAGE_NUM - pages的页段
    _Max_span->set_page_id(Span::ptr_to_id(_Ptr));
    _Max_span->set_page_count(MAX_PAGE_NUM - _Pages);

    // 新建一个页段用于返回
    Span * _Split_span = new Span();
    _Split_span->set_page_id(_Max_span->page_id() + MAX_PAGE_NUM - _Pages);
    _Split_span->set_page_count(_Pages);

    // 新页段插入页段链表中
    _Spans[_Max_span->page_count() - 1].push_front(_Max_span);

    // max_span页号插入空闲映射表
    _Free_span_map[_Max_span->page_id()] = _Max_span;
    _Free_span_map[_Max_span->page_id() + _Max_span->page_count() - 1] = _Max_span;

    // split_span页号插入繁忙映射表
    _Busy_span_map[_Split_span->page_id()] = _Split_span;
    _Busy_span_map[_Split_span->page_id() + _Split_span->page_count() - 1] = _Split_span;

    return _Split_span;
}

void PageCache::returnSpan(Span * _Span)
{
    std::lock_guard<std::mutex> _Lock(_Mutex);

    // 从繁忙映射表中删除该页段
    _Busy_span_map.erase(_Span->page_id());
    _Busy_span_map.erase(_Span->page_id() + _Span->page_count() - 1);

    // 向前寻找空闲的页
    auto _Prev_it = _Free_span_map.find(_Span->page_id() - 1);
    while (_Prev_it != _Free_span_map.end()) {
        Span * _Prev_span = _Prev_it->second;

        // 判断合并后是否超出上限
        if (_Span->page_count() + _Prev_span->page_count() > MAX_PAGE_NUM) {
            break;
        }

        // 从链表中删除该空闲页
        _Spans[_Prev_span->page_count() - 1].erase(_Prev_span);

        // 从哈希表中删除该空闲页
        _Free_span_map.erase(_Prev_span->page_id());
        _Free_span_map.erase(_Prev_span->page_id() + _Prev_span->page_count() - 1);

        // 合并页段
        _Span->set_page_id(_Prev_span->page_id());
        _Span->set_page_count(_Prev_span->page_count() + _Span->page_count());

        // 删除原空闲页
        delete _Prev_span;

        _Prev_it = _Free_span_map.find(_Span->page_id() - 1);
    }

    // 向后寻找空闲的页
    auto _Next_it = _Free_span_map.find(_Span->page_id() + _Span->page_count());
    while (_Next_it != _Free_span_map.end()) {
        Span * _Next_span = _Next_it->second;

        // 判断合并后是否超出上限
        if (_Span->page_count() + _Next_span->page_count() > MAX_PAGE_NUM) {
            break;
        }

        // 从链表中删除该空闲页
        _Spans[_Next_span->page_count() - 1].erase(_Next_span);

        // 从哈希表中删除该空闲页
        _Free_span_map.erase(_Next_span->page_id());
        _Free_span_map.erase(_Next_span->page_id() + _Next_span->page_count() - 1);

        // 合并页段，首页号不变，只需要调整大小
        _Span->set_page_count(_Next_span->page_count() + _Span->page_count());

        // 删除原空闲页
        delete _Next_span;

        _Next_it = _Free_span_map.find(_Span->page_id() + _Span->page_count());
    }

    // 添加新页段的映射
    _Free_span_map[_Span->page_id()] = _Span;
    _Free_span_map[_Span->page_id() + _Span->page_count() - 1] = _Span;

    // 合并完成，插入新的链表
    _Spans[_Span->page_count() - 1].push_front(_Span);
}

Span * PageCache::objectToSpan(void * _Ptr) noexcept
{
    size_type _Page_id = Span::ptr_to_id(_Ptr);

    std::lock_guard<std::mutex> _Lock(_Mutex);

    // 寻找首个大于该页号的迭代器
    auto _It = _Busy_span_map.upper_bound(_Page_id);

    if (_It == _Busy_span_map.begin()) {
        return nullptr;
    }

    --_It;
    Span * _Span = _It->second;

    if (_Page_id >= _Span->page_id() + _Span->page_count()) {
        // 不在该页段范围内
        return nullptr;
    }

    return _Span;
}

void * PageCache::_FetchFromSystem(size_type _Pages) const noexcept
{
    return Platform::aligned_malloc(PAGE_SIZE, _Pages << PAGE_SHIFT);
}

} // namespace WW
