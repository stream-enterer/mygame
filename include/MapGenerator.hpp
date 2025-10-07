#ifndef MAP_GENERATOR_HPP
#define MAP_GENERATOR_HPP

#include "Map.hpp"

namespace tutorial
{
    struct MapParameters
    {
        int maxRooms;
        int minRoomSize;
        int maxRoomSize;
        int width;
        int height;
    };

    class Map::Generator
    {
    public:
        Generator(const MapParameters& params);

        void Generate(Map& map);

    private:
        MapParameters params_;
    };
} // namespace tutorial

#endif // MAP_GENERATOR_HPP
