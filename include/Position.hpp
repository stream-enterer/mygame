#ifndef POSITION_HPP
#define POSITION_HPP

namespace tutorial
{
    struct pos_t
    {
        int x;
        int y;
    };

    constexpr bool operator==(pos_t lhs, pos_t rhs)
    {
        return (lhs.x == rhs.x && lhs.y == rhs.y);
    }

    constexpr bool operator!=(pos_t lhs, pos_t rhs)
    {
        return !(lhs == rhs);
    }

    constexpr pos_t operator+(pos_t lhs, pos_t rhs)
    {
        return pos_t { lhs.x + rhs.x, lhs.y + rhs.y };
    }

    constexpr pos_t& operator+=(pos_t& lhs, pos_t rhs)
    {
        lhs = lhs + rhs;
        return lhs;
    }

    constexpr pos_t operator-(pos_t lhs, pos_t rhs)
    {
        return pos_t { lhs.x - rhs.x, lhs.y - rhs.y };
    }

    constexpr pos_t operator/(pos_t lhs, int rhs)
    {
        return pos_t { lhs.x / rhs, lhs.y / rhs };
    }
} // namespace tutorial

#endif // POSITION_HPP
