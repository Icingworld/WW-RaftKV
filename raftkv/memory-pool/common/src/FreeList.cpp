#include "FreeList.h"

namespace WW
{

FreeObject::FreeObject()
    : _Next(nullptr)
{
}

FreeObject::FreeObject(FreeObject * _Next)
    : _Next(_Next)
{
}

FreeObject * FreeObject::next() const noexcept
{
    return _Next;
}

void FreeObject::set_next(FreeObject * _Next) noexcept
{
    this->_Next = _Next;
}

FreeListIterator::FreeListIterator(FreeObject * _Free_object) noexcept
    : _Free_object(_Free_object)
{
}

bool FreeListIterator::operator==(const FreeListIterator & _Other) const noexcept
{
    return _Free_object == _Other._Free_object;
}

bool FreeListIterator::operator!=(const FreeListIterator & _Other) const noexcept
{
    return _Free_object != _Other._Free_object;
}

FreeObject * FreeListIterator::operator*() noexcept
{
    return _Free_object;
}

FreeObject * FreeListIterator::operator->() noexcept
{
    return _Free_object;
}

FreeListIterator & FreeListIterator::operator++() noexcept
{
    _Free_object = _Free_object->next();
    return *this;
}

FreeListIterator FreeListIterator::operator++(int) noexcept
{
    FreeListIterator _Tmp = *this;
    ++*this;
    return _Tmp;
}

FreeList::FreeList()
    : _Head()
    , _Size(0)
    , _Max_size(1)
{
}

FreeObject * FreeList::front() noexcept
{
    return _Head.next();
}

void FreeList::push_front(FreeObject * _Free_object)
{
    _Free_object->set_next(_Head.next());
    _Head.set_next(_Free_object);
    ++_Size;
}

void FreeList::pop_front()
{
    FreeObject * _Next = _Head.next()->next();
    _Head.set_next(_Next);
    --_Size;
}

FreeList::iterator FreeList::begin() noexcept
{
    return iterator(_Head.next());
}

FreeList::iterator FreeList::end() noexcept
{
    return iterator(nullptr);
}

bool FreeList::empty() const noexcept
{
    return (_Head.next() == nullptr);
}

size_type FreeList::size() const noexcept
{
    return _Size;
}

size_type FreeList::max_size() const noexcept
{
    return _Max_size;
}

void FreeList::set_max_size(size_type _Max_size) noexcept
{
    this->_Max_size = _Max_size;
}

void FreeList::clear() noexcept
{
    _Head.set_next(nullptr);
    _Size = 0;
    _Max_size = 1;
}

} // namespace WW
