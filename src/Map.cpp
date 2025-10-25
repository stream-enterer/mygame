#include "Map.hpp"

#include "AiComponent.hpp"
#include "Colors.hpp"
#include "Entity.hpp"
#include "EntityManager.hpp"
#include "MapGenerator.hpp"
#include "Util.hpp"

#include <cmath>
#include <memory>

namespace tutorial
{
	Map::Map(int width, int height)
	    : tiles_(std::vector<tile_t>(width * height,
	                                 tile_t { TileType::WALL, false })),
	      console_(nullptr),
	      map_(nullptr),
	      width_(width),
	      height_(height),
	      currentScentValue_(SCENT_THRESHOLD)
	{
		// Create console using C API instead of C++ wrapper
		console_ = TCOD_console_new(width, height);
		if (!console_) {
			throw std::runtime_error(
			    "Failed to create map console");
		}

		// Create FOV map using C API instead of C++ wrapper
		map_ = TCOD_map_new(width, height);
		if (!map_) {
			TCOD_console_delete(console_);
			throw std::runtime_error("Failed to create FOV map");
		}
	}

	Map::~Map()
	{
		// Clean up manually - we're not using unique_ptr anymore
		if (console_) {
			TCOD_console_delete(console_);
		}
		if (map_) {
			TCOD_map_delete(map_);
		}
	}

	void Map::ComputeFov(pos_t origin, int fovRadius)
	{
		// Modern C API for FOV computation
		TCOD_map_compute_fov(map_, origin.x, origin.y, fovRadius, true,
		                     FOV_RESTRICTIVE);
	}

	void Map::Generate(Generator& generator)
	{
		Clear();
		generator.Generate(*this);
	}

	void Map::SetExplored(pos_t pos, bool explored)
	{
		tiles_.at(util::posToIndex(pos, width_)).explored = explored;
	}

	void Map::SetTileType(pos_t pos, TileType type)
	{
		tiles_.at(util::posToIndex(pos, width_)).type = type;

		switch (type) {
			case TileType::FLOOR:
				// transparent = true, walkable = true
				TCOD_map_set_properties(map_, pos.x, pos.y,
				                        true, true);
				break;
			case TileType::WALL:
				// transparent = false, walkable = false
				TCOD_map_set_properties(map_, pos.x, pos.y,
				                        false, false);
				break;
			default:
				break;
		}
	}

	void Map::Update()
	{
		// Clear console using C API
		TCOD_console_clear(console_);

		for (std::size_t i = 0; i < tiles_.size(); ++i) {
			const auto pos = util::indexToPos(i, width_);

			tcod::ColorRGB color { 0, 0, 0 }; // Default black
			auto& tile = tiles_.at(i);

			if (IsInFov(pos)) {
				tile.explored = true;

				switch (tile.type) {
					case TileType::FLOOR:
						color = color::light_amber;
						break;
					case TileType::WALL:
						color = color::dark_amber;
						break;
					default:
						break;
				}
			} else if (IsExplored(pos)) {
				switch (tile.type) {
					case TileType::FLOOR:
						color = color::light_azure;
						break;
					case TileType::WALL:
						color = color::dark_azure;
						break;
					default:
						break;
				}
			}

			// Set background color (0 = don't change character)
			TCOD_console_put_rgb(console_, pos.x, pos.y, 0, NULL,
			                     &color, TCOD_BKGND_SET);
		}
	}

	int Map::GetHeight() const
	{
		return height_;
	}

	const std::vector<Room>& Map::GetRooms() const
	{
		return rooms_;
	}

	TileType Map::GetTileType(pos_t pos) const
	{
		return tiles_.at(util::posToIndex(pos, width_)).type;
	}

	int Map::GetWidth() const
	{
		return width_;
	}

	bool Map::IsInBounds(pos_t pos) const
	{
		return (pos.x >= 0 && pos.y >= 0 && pos.x < width_
		        && pos.y < height_);
	}

	bool Map::IsExplored(pos_t pos) const
	{
		return tiles_.at(util::posToIndex(pos, width_)).explored;
	}

	bool Map::IsInFov(pos_t pos) const
	{
		// Use C API to check FOV
		return TCOD_map_is_in_fov(map_, pos.x, pos.y);
	}

	bool Map::IsWall(pos_t pos) const
	{
		// Use C API to check if walkable (walls are not walkable)
		return !TCOD_map_is_walkable(map_, pos.x, pos.y);
	}

	bool Map::IsTransparent(pos_t pos) const
	{
		// Use C API to check if transparent (walls block sight)
		return TCOD_map_is_transparent(map_, pos.x, pos.y);
	}

	void Map::Render(TCOD_Console* parent) const
	{
		// Use C API blit function - this copies our console to the
		// parent
		TCOD_console_blit(console_, 0, 0, width_, height_, parent, 0, 0,
		                  1.0f, 1.0f);
	}

	void Map::Clear()
	{
		rooms_.clear();
		TCOD_map_clear(
		    map_, false,
		    false); // Set all tiles to non-transparent, non-walkable

		for (auto& tile : tiles_) {
			tile.explored = false;
			tile.type = TileType::WALL;
		}
	}

	void Map::UpdateScent(pos_t playerPos)
	{
		// Increment scent value each turn
		currentScentValue_++;

		// Update scent in all visible tiles based on distance to player
		for (int x = 0; x < width_; ++x) {
			for (int y = 0; y < height_; ++y) {
				pos_t pos { x, y };
				if (IsInFov(pos)) {
					auto& tile = tiles_.at(
					    util::posToIndex(pos, width_));
					unsigned int oldScent = tile.scent;

					// Calculate Manhattan distance to
					// player
					int dx = std::abs(x - playerPos.x);
					int dy = std::abs(y - playerPos.y);
					int distance = dx + dy;

					// New scent = current turn value minus
					// distance
					unsigned int newScent =
					    currentScentValue_ - distance;

					// Only update if new scent is stronger
					// than old
					if (newScent > oldScent) {
						tile.scent = newScent;
					}
				}
			}
		}
	}

	unsigned int Map::GetScent(pos_t pos) const
	{
		if (!IsInBounds(pos)) {
			return 0;
		}
		return tiles_.at(util::posToIndex(pos, width_)).scent;
	}
} // namespace tutorial
