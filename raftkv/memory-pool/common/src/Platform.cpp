#include "Platform.h"

#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#elif defined(__linux__)
#include <cstdlib>
#endif

namespace WW
{

void * Platform::aligned_malloc(size_type _Alignment, size_type _Size)
{
    void * _Ptr = nullptr;

#if defined(_WIN32) || defined(_WIN64)
    _Ptr = _aligned_malloc(_Size, _Alignment);
#elif defined(__linux__)
    if (posix_memalign(&_Ptr, _Alignment, _Size) != 0) {
        _Ptr = nullptr;
    }
#endif

    return _Ptr;
}

void Platform::aligned_free(void * _Ptr)
{
#if defined(_WIN32) || defined(_WIN64)
    _aligned_free(_Ptr);
#elif defined(__linux__)
    std::free(_Ptr);
#endif
}

} // namespace WW
