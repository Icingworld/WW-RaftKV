#pragma once

#include <string>
#include <array>

/**
 * @brief 唯一标识符
 * @version UUIDv4
 */
class UUID
{
private:
    std::array<unsigned char, 16> _UUID;    // 128 位唯一标识符

public:
    UUID();

    ~UUID() = default;

public:
    /**
     * @brief 生成唯一标识符
     */
    void generate();

    /**
     * @brief 将唯一标识符转换为字符串
     */
    std::string toString() const;

    bool operator==(const UUID & other) const;

    bool operator!=(const UUID & other) const;
};
