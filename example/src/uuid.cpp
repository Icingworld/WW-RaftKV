#include "uuid.h"

#include <iomanip>
#include <sstream>
#include <random>

static std::random_device random_device;
static std::mt19937 gen(random_device());
static std::uniform_int_distribution<int> dist(0, 255);

UUID::UUID()
{
    generate();
}

void UUID::generate()
{
    for (auto & byte : _UUID) {
        byte = static_cast<unsigned char>(dist(gen));
    }

    _UUID[6] = (_UUID[6] & 0x0F) | 0x40;  // 设置版本号为 4
    _UUID[8] = (_UUID[8] & 0x3F) | 0x80;  // 设置变体为 RFC4122
}

std::string UUID::toString() const
{
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(_UUID[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            ss << "-";
        }
    }
    return ss.str();
}

bool UUID::operator==(const UUID & other) const
{
    return _UUID == other._UUID;
}

bool UUID::operator!=(const UUID & other) const
{
    return !(*this == other);
}
