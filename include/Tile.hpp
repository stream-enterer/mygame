#ifndef TILE_HPP
#define TILE_HPP

namespace tutorial
{
    enum class TileType
    {
        NONE,
        FLOOR,
        WALL
    };

    struct tile_t
    {
        TileType type;
        bool explored;
    };
} // namespace tutorial

#endif // TILE_HPP
