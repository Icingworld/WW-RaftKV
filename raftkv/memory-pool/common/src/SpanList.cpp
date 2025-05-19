#include "SpanList.h"

namespace WW
{

Span::Span()
    : _Free_list()
    , _Page_id(0)
    , _Prev(nullptr)
    , _Next(nullptr)
    , _Page_count(0)
    , _Used(0)
{
}

size_type Span::page_id() const noexcept
{
    return _Page_id;
}

void Span::set_page_id(size_type _Page_id) noexcept
{
    this->_Page_id = _Page_id;
}

size_type Span::page_count() const noexcept
{
    return _Page_count;
}

void Span::set_page_count(size_type _Page_count) noexcept
{
    this->_Page_count = _Page_count;
}

Span * Span::prev() const noexcept
{
    return _Prev;
}

void Span::set_prev(Span * _Prev) noexcept
{
    this->_Prev = _Prev;
}

Span * Span::next() const noexcept
{
    return _Next;
}

void Span::set_next(Span * _Next) noexcept
{
    this->_Next = _Next;
}

size_type Span::used() const noexcept
{
    return _Used;
}

void Span::set_used(size_type _Used) noexcept
{
    this->_Used = _Used;
}

FreeList * Span::get_free_list() noexcept
{
    return &_Free_list;
}

size_type Span::ptr_to_id(void * _Ptr) noexcept
{
    return reinterpret_cast<std::uintptr_t>(_Ptr) >> PAGE_SHIFT;
}

void * Span::id_to_ptr(size_type _Id) noexcept
{
    return reinterpret_cast<void *>(_Id << PAGE_SHIFT);
}

SpanListIterator::SpanListIterator(Span * _Span) noexcept
    : _Span(_Span)
{
}

bool SpanListIterator::operator==(const SpanListIterator & _Other) const noexcept
{
    return _Span == _Other._Span;
}

bool SpanListIterator::operator!=(const SpanListIterator & _Other) const noexcept
{
    return _Span != _Other._Span;
}

Span & SpanListIterator::operator*() noexcept
{
    return *_Span;
}

Span * SpanListIterator::operator->() noexcept
{
    return _Span;
}

SpanListIterator & SpanListIterator::operator++() noexcept
{
    _Span = _Span->next();
    return *this;
}

SpanListIterator SpanListIterator::operator++(int) noexcept
{
    SpanListIterator _Tmp = *this;
    ++*this;
    return _Tmp;
}

SpanListIterator & SpanListIterator::operator--() noexcept
{
    _Span = _Span->prev();
    return *this;
}

SpanListIterator SpanListIterator::operator--(int) noexcept
{
    SpanListIterator _Tmp = *this;
    --*this;
    return _Tmp;
}

SpanList::SpanList()
    : _Head()
    , _Mutex()
{
    _Head.set_next(&_Head);
    _Head.set_prev(&_Head);
}

Span & SpanList::front() noexcept
{
    return *_Head.next();
}

Span & SpanList::back() noexcept
{
    return *_Head.prev();
}

SpanList::iterator SpanList::begin() noexcept
{
    return iterator(_Head.next());
}

SpanList::iterator SpanList::end() noexcept
{
    return iterator(&_Head);
}

void SpanList::push_front(Span * _Span) noexcept
{
    Span * _Next = _Head.next();
    _Span->set_next(_Next);
    _Span->set_prev(&_Head);
    _Next->set_prev(_Span);
    _Head.set_next(_Span);
}

void SpanList::pop_front() noexcept
{
    Span * _Front = _Head.next();
    _Head.set_next(_Front->next());
    _Front->next()->set_prev(&_Head);
}

void SpanList::erase(Span * _Span) noexcept
{
    Span * _Prev = _Span->prev();
    Span * _Next = _Span->next();
    _Prev->set_next(_Next);
    _Next->set_prev(_Prev);
}

bool SpanList::empty() const noexcept
{
    return (_Head.next() == &_Head);
}

void SpanList::lock() noexcept
{
    _Mutex.lock();
}

void SpanList::unlock() noexcept
{
    _Mutex.unlock();
}

} // namespace WW
