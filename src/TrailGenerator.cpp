#include "TrailGenerator.hpp"

#include "Map.hpp"
#include "Tile.hpp"

#include <libtcod.h>

#include <algorithm>

namespace tutorial
{
	inline namespace
	{
		// Clamp value between min and max
		int Clamp(int value, int min, int max)
		{
			if (value < min) return min;
			if (value > max) return max;
			return value;
		}

		// Choose random corridor length
		int ChooseCorridorLength(const TrailConfig& config)
		{
			auto* rand = TCODRandom::getInstance();
			return rand->getInt(config.minLength, config.maxLength);
		}

		// Calculate directional bias away from map edges
		// Returns -1, 0, or 1 indicating preferred direction
		int GetEdgeBias(int current, int min, int max, int margin)
		{
			// Too close to min edge? Prefer moving toward max
			if (current < min + margin) {
				return 1;
			}
			// Too close to max edge? Prefer moving toward min
			if (current > max - margin) {
				return -1;
			}
			// Safe distance from edges, no bias
			return 0;
		}
	} // namespace

	TrailConfig GetDefaultTrailConfig()
	{
		return TrailConfig {
			15,  // edgeMargin (DCSS uses 15)
			3,   // minLength
			10,  // maxLength
			0.3f // intersectChance (30% chance to turn early)
		};
	}

	std::vector<pos_t> GenerateTrail(Map& map, pos_t start, pos_t end,
	                                 const TrailConfig& config)
	{
		std::vector<pos_t> carved;
		auto* rand = TCODRandom::getInstance();

		pos_t current = start;
		carved.push_back(current);

		// Carve the starting position
		if (map.IsWall(current)) {
			map.SetTileType(current, TileType::FLOOR);
		}

		// Keep walking until we reach the end (or get very close)
		while (std::abs(current.x - end.x) > 1
		       || std::abs(current.y - end.y) > 1) {
			// Decide whether to move horizontally or vertically
			int dx = end.x - current.x;
			int dy = end.y - current.y;

			// Get edge biases for both axes
			int xBias =
			    GetEdgeBias(current.x, 0, map.GetWidth() - 1,
			                config.edgeMargin);
			int yBias =
			    GetEdgeBias(current.y, 0, map.GetHeight() - 1,
			                config.edgeMargin);

			// Choose axis to move along
			bool moveHorizontal;

			if (dx == 0) {
				// Must move vertically
				moveHorizontal = false;
			} else if (dy == 0) {
				// Must move horizontally
				moveHorizontal = true;
			} else {
				// Can move either way - consider biases and
				// randomness
				float horizontalWeight =
				    std::abs(dx) + xBias * 3.0f;
				float verticalWeight =
				    std::abs(dy) + yBias * 3.0f;
				float totalWeight =
				    horizontalWeight + verticalWeight;

				float roll = rand->getFloat(0.0f, totalWeight);
				moveHorizontal = (roll < horizontalWeight);
			}

			// Choose corridor length for this segment
			int corridorLength = ChooseCorridorLength(config);

			// Carve the corridor
			for (int step = 0; step < corridorLength; ++step) {
				// Determine direction
				pos_t dir { 0, 0 };

				if (moveHorizontal) {
					if (dx > 0)
						dir.x = 1;
					else if (dx < 0)
						dir.x = -1;
				} else {
					if (dy > 0)
						dir.y = 1;
					else if (dy < 0)
						dir.y = -1;
				}

				// Move
				pos_t next = current + dir;

				// Clamp to map bounds
				next.x = Clamp(next.x, 0, map.GetWidth() - 1);
				next.y = Clamp(next.y, 0, map.GetHeight() - 1);

				// Check if we hit an existing floor
				// (intersection)
				bool hitFloor = !map.IsWall(next);

				// Carve floor
				if (map.IsWall(next)) {
					map.SetTileType(next, TileType::FLOOR);
				}

				current = next;
				carved.push_back(current);

				// Update remaining distance
				dx = end.x - current.x;
				dy = end.y - current.y;

				// Stop if we reached the end
				if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
					break;
				}

				// Early exit from corridor if we intersect and
				// roll succeeds
				if (hitFloor
				    && rand->getFloat(0.0f, 1.0f)
				           < config.intersectChance) {
					break;
				}
			}

			// Safety check to prevent infinite loops
			if (current == end) {
				break;
			}
		}

		// Make sure end position is carved
		if (map.IsWall(end)) {
			map.SetTileType(end, TileType::FLOOR);
		}
		carved.push_back(end);

		return carved;
	}

} // namespace tutorial
