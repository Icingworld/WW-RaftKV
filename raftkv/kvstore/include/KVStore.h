#pragma once

#include <SkipList.h>

namespace WW
{

/**
 * @brief KV储存
 * @tparam _Key 键类型
 * @tparam _Value 值类型
 */
template <
    typename _Ty_key,
    typename _Ty_value
> class KVStore
{
public:
    using key_type = _Ty_key;
    using value_type = _Ty_value;
    using pair_type = std::pair<const _Ty_key, _Ty_value>;
    using size_type = std::size_t;
    using level_type = int;

protected:
    _Skiplist<key_type, value_type> _Skiplist;      // 跳表

public:
    KVStore() = default;

    explicit KVStore(level_type _Max_level)
        : _Skiplist(_Max_level)
    {
    }

    ~KVStore() = default;

public:
    /**
     * @brief 获取值
     * @param _Key 键
     * @return 值
     */
    const value_type & get(const key_type & _Key) noexcept
    {
        return _Skiplist[_Key];
    }

    /**
     * @brief 插入键值对
     * @param _Key 键
     * @param _Value 值
     * @return 是否插入成功
     */
    bool put(const key_type & _Key, const value_type & _Value) noexcept
    {
        auto pair = _Skiplist.insert(pair_type(_Key, _Value));
        return pair.second;
    }

    /**
     * @brief 更新键值对
     * @param _Key 键
     * @param _Value 值
     * @return 是否更新成功
     */
    bool update(const key_type & _Key, const value_type & _Value) noexcept
    {
        _Skiplist[_Key] = _Value;
        return true;
    }

    /**
     * @brief 删除键值对
     * @param _Key 键
     * @return 是否删除成功
     */
    bool remove(const key_type & _Key) noexcept
    {
        auto count = _Skiplist.erase(_Key);
        return count != 0;
    }

    /**
     * @brief 查询键是否存在
     * @param _Key 键
     * @return 是否存在
     */
    bool contains(const key_type & _Key) const noexcept
    {
        return _Skiplist.contains(_Key);
    }

    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    bool empty() const noexcept
    {
        return _Skiplist.empty();
    }

    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    size_type size() const noexcept
    {
        return _Skiplist.size();
    }
};

} // namespace WW