#pragma once

#include <cstdlib>
#include <cstddef>
#include <utility>
#include <vector>

#include <KVCommon.h>

namespace WW
{

/**
 * @brief 跳表
 * @tparam _Key 键类型
 * @tparam _Value 值类型
 */
template <
    typename _Ty_key,
    typename _Ty_value
> class _Skiplist;

/**
 * @brief 跳表节点
 * @tparam _Key 键类型
 * @tparam _Value 值类型
 */
template <
    typename _Ty_key,
    typename _Ty_value
> class _Skip_list_node
{
public:
    using key_type = _Ty_key;
    using value_type = _Ty_value;
    using pair_type = std::pair<const _Ty_key, _Ty_value>;
    using level_type = int;
    using node_pointer = _Skip_list_node<_Ty_key, _Ty_value> *;

private:
    pair_type _Data;                        // 键值对
    std::vector<node_pointer> _Forward;     // 向前数组

public:
    _Skip_list_node()
        : _Skip_list_node(pair_type(), 0)
    {
    }

    explicit _Skip_list_node(level_type _Level)
        : _Skip_list_node(pair_type(), _Level)
    {
    }

    _Skip_list_node(const pair_type & _Pair, level_type _Level)
        : _Data(_Pair)
        , _Forward(_Level + 1, nullptr)
    {
    }

    ~_Skip_list_node() = default;

public:
    /**
     * @brief 获取键值对
     * @return 键值对
     */
    pair_type & data() noexcept
    {
        return _Data;
    }

    /**
     * @brief 获取层级
     * @return 层级
     */
    const level_type level() const noexcept
    {
        return _Forward.size() - 1;
    }

    /**
     * @brief 获取该节点在指定层级的向前节点
     * @param _Level 层级
     * @return 节点指针
     */
    node_pointer & forward(level_type _Level) noexcept
    {
        return _Forward[_Level];
    }

    /**
     * @brief 获取该节点在指定层级的向前节点
     * @param _Level 层级
     * @return 节点指针
     */
    const node_pointer & forward(level_type _Level) const noexcept
    {
        return _Forward[_Level];
    }

    /**
     * @brief 设置值
     * @param value 值
     */
    void set_value(const value_type & value) noexcept
    {
        _Data.second = value;
    }
};

/**
 * @brief 跳表常量迭代器
 */
template <
    typename _Ty_key,
    typename _Ty_value
> class _Skiplist_const_iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using key_type = _Ty_key;
    using value_type = _Ty_value;
    using pair_type = std::pair<const _Ty_key, _Ty_value>;

    using node_type = _Skip_list_node<key_type, value_type>;
    using node_pointer = _Skip_list_node<key_type, value_type> *;
    using self = _Skiplist_const_iterator<_Ty_key, _Ty_value>;

protected:
    node_pointer _Node;             // 节点指针

public:
    _Skiplist_const_iterator()
        : _Node(nullptr)
    { //空迭代器
    }

    _Skiplist_const_iterator(const node_pointer _Node)
        : _Node(const_cast<node_pointer>(_Node))
    {
    }

public:
    const pair_type & operator*() const noexcept
    {
        return _Node->data();
    }

    const pair_type * operator->() const noexcept
    {
        return &(operator*());
    }

    self & operator++() noexcept
    {
        // 只需要在第0层级移动
        _Node = _Node->forward(0);
        return *this;
    }

    self operator++(int) noexcept
    {
        self _Tmp = *this;
        ++*this;
        return _Tmp;
    }

    bool operator==(const self & _Other) const noexcept
    {
        return _Node == _Other._Node;
    }

    bool operator!=(const self & _Other) const noexcept
    {
        return !(*this == _Other);
    }

protected:
    // 允许跳表类访问私有成员
    friend class _Skiplist<_Ty_key, _Ty_value>;

    /**
     * @brief 获取迭代器所持有的节点指针
     * @return 节点指针
     */
    node_pointer _Get_node() noexcept
    {
        return _Node;
    }
};

/**
 * @brief 跳表迭代器
 */
template <
    typename _Ty_key,
    typename _Ty_value
> class _Skiplist_iterator : public _Skiplist_const_iterator<_Ty_key, _Ty_value>
{
public:
    using iterator_category = std::forward_iterator_tag;
    using key_type = _Ty_key;
    using value_type = _Ty_value;
    using pair_type = std::pair<const _Ty_key, _Ty_value>;

    using node_type = _Skip_list_node<key_type, value_type>;
    using node_pointer = _Skip_list_node<key_type, value_type> *;
    using self = _Skiplist_iterator<_Ty_key, _Ty_value>;
    using base = _Skiplist_const_iterator<_Ty_key, _Ty_value>;

public:
    _Skiplist_iterator()
        : base()
    { //空迭代器
    }

    _Skiplist_iterator(node_pointer _Node)
        : base(_Node)
    {
    }

public:
    pair_type & operator*() const noexcept
    {
        return const_cast<pair_type &>(base::operator*());
    }

    pair_type * operator->() const noexcept
    {
        return const_cast<pair_type *>(base::operator->());
    }

    self & operator++() noexcept
    {
        base::operator++();
        return *this;
    }

    self operator++(int) noexcept
    {
        self _Tmp = *this;
        ++*this;
        return _Tmp;
    }
};

/**
 * @brief 跳表
 * @tparam _Key 键类型
 * @tparam _Value 值类型
 */
template <
    typename _Ty_key,
    typename _Ty_value
> class _Skiplist
{
public:
    using key_type = _Ty_key;
    using value_type = _Ty_value;
    using pair_type = std::pair<const _Ty_key, _Ty_value>;
    using size_type = std::size_t;
    using iterator = _Skiplist_iterator<key_type, value_type>;
    using const_iterator = _Skiplist_const_iterator<key_type, value_type>;

    using level_type = int;
    using node_type = _Skip_list_node<key_type, value_type>;
    using node_pointer = _Skip_list_node<key_type, value_type> *;

private:
    node_pointer _Head;                 // 头节点
    level_type _Max_level_index;        // 最大层级索引
    level_type _Current_level_index;    // 当前最高层级索引
    size_type _Size;                    // 节点个数

public:
    _Skiplist()
        : _Skiplist(MAX_LEVEL)
    {
    }

    explicit _Skiplist(level_type _Max_level)
        : _Head(_Create_node(_Max_level - 1))
        , _Max_level_index(_Max_level - 1)
        , _Current_level_index(0)
        , _Size(0)
    {
    }

    ~_Skiplist()
    {
        // 删除所有节点
        clear();

        // 删除头节点
        _Destroy_node(_Head);
    }

public:
    // 元素访问

    /**
     * @brief 带越界检查访问指定的元素
     */
    value_type & at(const key_type & _Key)
    {
        node_pointer _Ptr = _Find(_Key);
        if (_Ptr == nullptr) {
            _Throw_out_of_range();
        }

        return _Ptr->data().second;
    }

    /**
     * @brief 带越界检查访问指定的元素
     */
    const value_type & at(const key_type & _Key) const
    {
        node_pointer _Ptr = _Find(_Key);
        if (_Ptr == nullptr) {
            _Throw_out_of_range();
        }

        return _Ptr->data().second;
    }

    /**
     * @brief 访问或插入指定的元素
     */
    value_type & operator[](const key_type & _Key)
    {
        return _Get_or_insert(_Key);
    }

    /**
     * @brief 访问或插入指定的元素
     */
    value_type & operator[](key_type && _Key)
    {
        return _Get_or_insert(std::move(_Key));
    }

    // 迭代器

    /**
     * @brief 返回指向起始的迭代器
     */
    iterator begin() noexcept
    {
        return ++iterator(_Head);
    }

    /**
     * @brief 返回指向起始的迭代器
     */
    const_iterator begin() const noexcept
    {
        return ++const_iterator(_Head);
    }

    /**
     * @brief 返回指向起始的迭代器
     */
    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    /**
     * @brief 返回指向末尾的迭代器
     */
    iterator end() noexcept
    {
        return iterator(nullptr);
    }

    /**
     * @brief 返回指向末尾的迭代器
     */
    const_iterator end() const noexcept
    {
        return const_iterator(nullptr);
    }

    /**
     * @brief 返回指向末尾的迭代器
     */
    const_iterator cend() const noexcept
    {
        return end();
    }

    // 容量

    /**
     * @brief 获取跳表大小
     * @return 跳表大小
     */
    size_type size() const noexcept
    {
        return _Size;
    }

    /**
     * @brief 判断跳表是否为空
     * @return 是否为空
     */
    bool empty() const noexcept
    {
        return _Size == 0;
    }

    // 修改器

    /**
     * @brief 清空跳表
     */
    void clear() noexcept
    {
        node_pointer _Cur = _Head->forward(0);

        while (_Cur != nullptr) {
            node_pointer _Next = _Cur->forward(0);
            _Destroy_node(_Cur);
            _Cur = _Next;
        }

        for (level_type _Level = 0; _Level <= _Current_level_index; ++_Level) {
            _Head->forward(_Level) = nullptr;
        }

        _Current_level_index = 0;
        _Size = 0;
    }

    /**
     * @brief 当不存在键时，插入一个键值对
     * @param _Pair 键值对
     * @return `std::pair<iterator, bool>`
     */
    std::pair<iterator, bool> insert(const pair_type & _Pair) noexcept
    {
        return _Insert(_Pair);
    }

    /**
     * @brief 当不存在键时，插入一个键值对
     * @param _Pair 键值对
     * @return `std::pair<iterator, bool>`
     */
    std::pair<iterator, bool> insert(pair_type && _Pair) noexcept
    {
        return _Insert(std::move(_Pair));
    }

    /**
     * @brief 擦除元素
     * @param _Pos 迭代器
     * @return 后随被删除元素的迭代器
     */
    iterator erase(const_iterator _Pos)
    {
        // 由于跳表的特性，_Pos代表的节点有自己的最高层级
        // 只需要在这些层级中删除即可

        if (_Pos == end()) {
            return end();
        }

        level_type _Pos_max_level_index = _Pos._Get_node()->level();

        node_pointer _Cur = nullptr;
        for (level_type _Level = _Pos_max_level_index; _Level > 0; --_Level) {
            // 从头节点开始
            _Cur = _Head;

            while (_Cur->forward(_Level) != _Pos._Get_node()) {
                _Cur = _Cur->forward(_Level);
            }

            // 找到该节点的前驱节点，开始跳过
            _Cur->forward(_Level) = _Cur->forward(_Level)->forward(_Level);
        }

        // 第0层手动删除，因为需要返回迭代器
        _Cur = _Head;

        while (_Cur->forward(0) != _Pos._Get_node()) {
            _Cur = _Cur->forward(0);
        }

        node_pointer _Next_ptr = _Cur->forward(0)->forward(0);
        _Cur->forward(0) = _Next_ptr;

        // 删除节点
        _Destroy_node(_Pos._Get_node());

        // 检查是否需要降低跳表的高度
        while (_Current_level_index > 0 && _Head->forward(_Current_level_index) == nullptr) {
            --_Current_level_index;
        }

        // 更新节点个数
        --_Size;

        // 返回该节点迭代器
        return iterator(_Next_ptr);
    }

    /**
     * @brief 删除指定键的元素
     * @param _Key 键
     * @return 被删除的元素个数
     */
    size_type erase(const key_type & _Key) noexcept
    {
        std::vector<node_pointer> _Update_list(_Max_level_index + 1, nullptr);
        node_pointer _Ptr = _Find_with_update(_Key, _Update_list);

        if (_Ptr == nullptr || _Ptr->data().first != _Key) {
            // 不存在这个节点，删除失败
            return 0;
        }

        // 开始删除节点
        for (level_type _Level = 0; _Level <= _Current_level_index; ++_Level) {
            if (_Update_list[_Level]->forward(_Level) != _Ptr) {
                // 从这里开始上层都没有该键值对了
                break;
            }

            // 是该键值对，删除
            _Update_list[_Level]->forward(_Level) = _Ptr->forward(_Level);
        }

        // 删除节点
        _Destroy_node(_Ptr);

        // 检查是否需要降低跳表的高度
        while (_Current_level_index > 0 && _Head->forward(_Current_level_index) == nullptr) {
            --_Current_level_index;
        }

        --_Size;

        return 1;
    }

    // 查找

    /**
     * @brief 寻找带有特定键的元素
     * @param _Key 键
     * @return 迭代器
     */
    iterator find(const key_type & _Key) noexcept
    {
        return iterator(_Find(_Key));
    }

    /**
     * @brief 寻找带有特定键的元素
     * @param _Key 键
     * @return 迭代器
     */
    const_iterator find(const key_type & _Key) const noexcept
    {
        return const_iterator(_Find(_Key));
    }

    /**
     * @brief 查询一个键是否存在
     * @param _Key 键
     * @return 是否存在
     */
    bool contains(const key_type & _Key) const noexcept
    {
        node_pointer _Node_ptr = _Find(_Key);
        return _Node_ptr != nullptr;
    }

private:
    /**
     * @brief 随机生成一个层级
     * @return 层级
     */
    level_type _Random_level() const noexcept
    {
        level_type _Level = 1;

        while (rand() % 2 && _Level < _Max_level_index) {
            ++_Level;
        }

        return _Level;
    }

    /**
     * @brief 创建一个空节点
     * @return 节点指针
     */
    node_pointer _Create_node() const noexcept
    {
        return new node_type();
    }

    /**
     * @brief 创建一个指定层级的空节点
     * @param _Level 层级
     * @return 节点指针
     */
    node_pointer _Create_node(level_type _Level) const noexcept
    {
        return new node_type(_Level);
    }

    /**
     * @brief 创建一个节点
     * @param _Pair 键值对
     * @param _Level 层级
     * @return 节点指针
     */
    node_pointer _Create_node(const pair_type & _Pair, level_type _Level) const noexcept
    {
        return new node_type(_Pair, _Level);
    }

    /**
     * @brief 销毁一个节点
     * @param _Node 节点指针
     */
    void _Destroy_node(node_pointer _Node) const noexcept
    {
        delete _Node;
    }

    /**
     * @brief 查找一个节点
     * @param _Key 键
     * @return 节点指针
     */
    node_pointer _Find(const key_type & _Key) const noexcept
    {
        // 从头节点开始查找
        node_pointer _Cur = _Head;

        // 从当前最高层级开始查找
        for (level_type _Level = _Current_level_index; _Level >= 0; --_Level) {
            // 在当前层级中前进，直到找到大于等于_key的节点
            while (_Cur->forward(_Level) != nullptr && _Cur->forward(_Level)->data().first < _Key) {
                _Cur = _Cur->forward(_Level);
            }
        }
        
        // 到达0层，向后移动一个就是最终找到的节点
        _Cur = _Cur->forward(0);

        // 判断该节点是不是目标节点
        if (_Cur != nullptr && _Cur->data().first == _Key) {
            return _Cur;
        }

        return nullptr;
    }

    /**
     * @brief 带前驱记录的查找节点
     * @param _Key 键
     * @param _Update_list 前驱记录数组
     * @return 节点指针
     */
    node_pointer _Find_with_update(const key_type & _Key, std::vector<node_pointer> & _Update_list) const noexcept
    {
        node_pointer _Cur = _Head;

        // 从顶层向下查找，记录每一层的前驱
        for (level_type _Level = _Current_level_index; _Level >= 0; --_Level) {
            while (_Cur->forward(_Level) != nullptr && _Cur->forward(_Level)->data().first < _Key) {
                _Cur = _Cur->forward(_Level);
            }

            _Update_list[_Level] = _Cur;
        }

        // 到达第0层，下一个就是目标节点
        _Cur = _Cur->forward(0);

        return _Cur;
    }

    /**
     * @brief 创建节点并插入
     * @param _Pair 键值对
     * @param _Update_list 前驱记录数组
     * return 节点指针
     */
    node_pointer _Create_and_insert(const pair_type & _Pair, std::vector<node_pointer> & _Update_list)
    {
        // 创建新节点
        level_type _New_level_index = _Random_level() - 1;
        node_pointer _New_node = _Create_node(_Pair, _New_level_index);

        // 如果新节点层级大于当前最高层级，将多出来的这些层级加入更新列表中
        if (_New_level_index > _Current_level_index) {
            for (level_type _Level = _Current_level_index + 1; _Level <= _New_level_index; ++_Level) {
                // 由于跳表的开头必须是头节点，所以多出来的高度的前置都指向头节点即可
                _Update_list[_Level] = _Head;
            }

            // 更新最高层级
            _Current_level_index = _New_level_index;
        }

        // 开始遍历各层的前置节点并修改
        for (level_type _Level = 0; _Level <= _New_level_index; ++_Level) {
            // 将原来的指针添加到新节点中
            _New_node->forward(_Level) = _Update_list[_Level]->forward(_Level);
            // 将新节点添加到前置节点的向前列表中
            _Update_list[_Level]->forward(_Level) = _New_node;
        }

        ++_Size;

        return _New_node;
    }

    /**
     * @brief 当不存在键时，插入一个键值对
     * @param _Pair 键值对
     * @return `std::pair<iterator, bool>`
     */
    template<typename _P>
    std::pair<iterator, bool> _Insert(_P && _Pair)
    {
        // 使用一个数组来储存前一个结点的向前指针，为了避免越界，直接初始化大小为最大大小
        std::vector<node_pointer> _Update_list(_Max_level_index + 1, nullptr);
        node_pointer _Ptr = _Find_with_update(_Pair.first, _Update_list);

        // 判断是否已经存在
        if (_Ptr != nullptr && _Ptr->data().first == _Pair.first) {
            return {iterator(_Ptr), false};
        }

        // 不存在，创建并插入新节点
        _Ptr = _Create_and_insert(std::forward<_P>(_Pair), _Update_list);

        return {iterator(_Ptr), true};
    }

    /**
     * @brief 获取值或插入
     * @param _Key 键
     * return 值
     */
    template <typename _K>
    value_type & _Get_or_insert(_K && _Key)
    {
        std::vector<node_pointer> _Update_list(_Max_level_index + 1, nullptr);
        node_pointer _Ptr = _Find_with_update(_Key, _Update_list);
        if (_Ptr == nullptr || _Ptr->data().first != _Key) {
            // 不存在该键，创建并插入
            _Ptr = _Create_and_insert({_Key, value_type()}, _Update_list);
        }

        return _Ptr->data().second;
    }

    /**
     * @brief 抛出越界异常
     */
    [[noreturn]] void _Throw_out_of_range() const
    {
        throw std::out_of_range("skiplist key not found");
    }
};

} // namespace WW