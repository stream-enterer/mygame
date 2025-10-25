#ifndef TILE_HPP
#define TILE_HPP

namespace tutorial
{
	enum class TileType { NONE, FLOOR, WALL };

	struct tile_t {
		TileType type;
		bool explored;
		unsigned int scent; // Amount of player scent on this tile

		tile_t() : type(TileType::NONE), explored(false), scent(0)
		{
		}
		tile_t(TileType t, bool e) : type(t), explored(e), scent(0)
		{
		}
	};
} // namespace tutorial

#endif // TILE_HPP
