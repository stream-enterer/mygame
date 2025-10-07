#include "Room.hpp"

namespace tutorial
{
    Room::Room(pos_t origin, int width, int height)
        : origin_(origin), end_(pos_t { origin.x + width, origin.y + height })
    {
    }

    pos_t Room::GetCenter() const
    {
        return ((origin_ + end_) / 2);
    }

    pos_t Room::GetEnd() const
    {
        return end_;
    }

    pos_t Room::GetOrigin() const
    {
        return origin_;
    }

    std::vector<pos_t> Room::GetInner() const
    {
        std::vector<pos_t> inner;

        for (int x = origin_.x + 1; x < end_.x; ++x)
        {
            for (int y = origin_.y + 1; y < end_.y; ++y)
            {
                inner.push_back(pos_t { x, y });
            }
        }

        return inner;
    }

    bool Room::Intersects(const Room& other) const
    {
        return (origin_.x <= other.end_.x && end_.x >= other.origin_.x
                && origin_.y <= other.end_.y && end_.y >= other.origin_.y);
    }
} // namespace tutorial
