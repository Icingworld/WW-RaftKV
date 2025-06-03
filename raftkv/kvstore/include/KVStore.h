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
    using iterator = typename _Skiplist<key_type, value_type>::iterator;

protected:
    _Skiplist<key_type, value_type> _Skip_list;      // 跳表

public:
    KVStore() = default;

    explicit KVStore(level_type _Max_level)
        : _Skip_list(_Max_level)
    {
    }

    ~KVStore() = default;

public:
    iterator begin()
    {
        return _Skip_list.begin();
    }

    iterator end()
    {
        return _Skip_list.end();
    }

    /**
     * @brief 获取值
     * @param _Key 键
     * @return 值
     */
    const value_type & get(const key_type & _Key) noexcept
    {
        return _Skip_list[_Key];
    }

    /**
     * @brief 插入键值对
     * @param _Key 键
     * @param _Value 值
     * @return 是否插入成功
     */
    bool put(const key_type & _Key, const value_type & _Value) noexcept
    {
        auto pair = _Skip_list.insert(pair_type(_Key, _Value));
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
        _Skip_list[_Key] = _Value;
        return true;
    }

    /**
     * @brief 删除键值对
     * @param _Key 键
     * @return 是否删除成功
     */
    bool remove(const key_type & _Key) noexcept
    {
        auto count = _Skip_list.erase(_Key);
        return count != 0;
    }

    /**
     * @brief 查询键是否存在
     * @param _Key 键
     * @return 是否存在
     */
    bool contains(const key_type & _Key) const noexcept
    {
        return _Skip_list.contains(_Key);
    }

    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    bool empty() const noexcept
    {
        return _Skip_list.empty();
    }

    /**
     * @brief 清空
    */
    void clear() noexcept
    {
        _Skip_list.clear();
    }

    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    size_type size() const noexcept
    {
        return _Skip_list.size();
    }
};

} // namespace WW