#include "Size.h"

namespace WW
{

size_type Size::index_to_size(size_type _Index) noexcept
{
    if (_Index <= 15) {
        // 8 * (_Index + 1)
        return (_Index + 1) * 8;
    } else if (_Index <= 71) {
        // 144 + 16 * (_Index - 16)
        return 128 + 16 * (_Index - 15);
    } else if (_Index <= 127) {
        // 1152 + 128 * (_Index - 72)
        return 1024 + 128 * (_Index - 71);
    } else if (_Index <= 183) {
        // 9216 + 1024 * (_Index - 128)
        return 8192 + 1024 * (_Index - 127);
    } else if (_Index <= 207) {
        // 73728 + 8192 * (_Index - 184)
        return 65536 + 8192 * (_Index - 183);
    } else {
        // 不存在这种情况
        return 0;
    }
}

size_type Size::size_to_index(size_type _Size) noexcept
{
    if (_Size <= 128) {
        // 8, 16, ..., 128 → 共16类，索引0~15
        return (_Size + 7) / 8 - 1;
    } else if (_Size <= 1024) {
        // 144, 160, ..., 1024 → 步长16，共56类，索引16~71
        return 16 + (_Size - 129) / 16;
    } else if (_Size <= 8192) {
        // 1152, 1280, ..., 8192 → 步长128，共56类，索引72~127
        return 72 + (_Size - 1025) / 128;
    } else if (_Size <= 65536) {
        // 9216, 10240, ..., 65536 → 步长1024，共56类，索引128~183
        return 128 + (_Size - 8193) / 1024;
    } else if (_Size <= 262144) {
        // 73728, 81920, ..., 262144 → 步长8192，共24类，索引184~207
        return 184 + (_Size - 65537) / 8192;
    } else {
        // 不存在这种情况
        return 0;
    }
}

size_type Size::round_up(size_type _Size) noexcept
{
    if (_Size <= 128) {
        // [0, 128]，按照8字节对齐
        return (_Size + 8 - 1) & ~(8 - 1);
    } else if (_Size <= 1024) {
        // [129, 1024]，按照16字节对齐
        return (_Size + 16 - 1) & ~(16 - 1);
    } else if (_Size <= 8192) {
        // [1025, 8192]，按照128字节对齐
        return (_Size + 128 - 1) & ~(128 - 1);
    } else if (_Size <= 65536) {
        // [8193, 65536]，按照1024字节对齐
        return (_Size + 1024 - 1) & ~(1024 - 1);
    } else {
        // [65537, 262144]，按照8192字节对齐
        return (_Size + 8192 - 1) & ~(8192 - 1);
    }
}

} // namespace WW
