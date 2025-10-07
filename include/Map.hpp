#ifndef MAP_HPP
#define MAP_HPP

#include "Position.hpp"
#include "Room.hpp"
#include "Tile.hpp"

#include <libtcod.h>

#include <vector>

namespace tutorial
{
    class Map
    {
    public:
        class Generator;

        Map(int width, int height);
        ~Map();  // Now we need destructor to clean up C API resources

        void ComputeFov(pos_t origin, int fovRadius);
        void Generate(Generator& generator);
        void SetExplored(pos_t pos, bool explored);
        void SetTileType(pos_t pos, TileType type);
        void Update();

        int GetHeight() const;
        const std::vector<Room>& GetRooms() const;
        const std::vector<tile_t>& GetTiles() const { return tiles_; }
        TileType GetTileType(pos_t pos) const;
        int GetWidth() const;
        bool IsExplored(pos_t pos) const;
        bool IsInBounds(pos_t pos) const;
        bool IsInFov(pos_t pos) const;
        bool IsWall(pos_t pos) const;
        void Render(TCOD_Console* parent) const;  // Changed parameter type

    private:
        void Clear();

        std::vector<Room> rooms_;
        std::vector<tile_t> tiles_;

        // Changed from unique_ptr to raw pointers - we manage lifecycle manually
        TCOD_Console* console_;
        TCOD_Map* map_;

        int width_;
        int height_;
    };
} // namespace tutorial

#endif // MAP_HPP
