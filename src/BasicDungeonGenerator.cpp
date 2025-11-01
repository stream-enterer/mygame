#include "BasicDungeonGenerator.hpp"

#include "MapGenerator.hpp"
#include "PathFinding.hpp"
#include "Room.hpp"
#include "Tile.hpp"

#include <libtcod.h>

#include <algorithm>

namespace tutorial
{
	inline namespace
	{
		// Pick a random position within bounds, with margin from edges
		pos_t PickRandomPosition(int width, int height, int margin)
		{
			auto* rand = TCODRandom::getInstance();
			return pos_t { rand->getInt(margin, width - margin - 1),
				       rand->getInt(margin,
				                    height - margin - 1) };
		}

		// Check if a room would overlap with existing rooms
		bool RoomOverlaps(const Room& newRoom,
		                  const std::vector<Room>& existingRooms)
		{
			for (const auto& room : existingRooms) {
				if (newRoom.Intersects(room)) {
					return true;
				}
			}
			return false;
		}
	} // namespace

	BasicDungeonGenerator::BasicDungeonGenerator(
	    const BasicDungeonConfig& config)
	    : Map::Generator(MapParameters {
	          config.maxRooms, config.minRoomSize, config.maxRoomSize,
	          0, // width - will be set when Generate() is called
	          0  // height - will be set when Generate() is called
	      }),
	      config_(config)
	{
	}

	BasicDungeonConfig BasicDungeonGenerator::GetDefaultConfig(int width,
	                                                           int height)
	{
		// Suppress unused parameter warnings (reserved for future use)
		(void)width;
		(void)height;

		return BasicDungeonConfig {
			3,                      // numTrails (DCSS default)
			5,                      // minRooms
			30,                     // maxRooms (DCSS does 5-30)
			4,                      // minRoomSize
			10,                     // maxRoomSize
			GetDefaultTrailConfig() // Trail configuration
		};
	}

	void BasicDungeonGenerator::Generate(Map& map)
	{
		trails_.clear();

		// Phase 1: Generate winding trails
		GenerateTrails(map);

		// Phase 2: Connect all trail endpoints
		ConnectTrails(map);

		// Phase 3: Place rooms over the structure
		PlaceRooms(map);

		// Note: Door placement will come in Phase 4
	}

	void BasicDungeonGenerator::GenerateTrails(Map& map)
	{
		const int margin = config_.trailConfig.edgeMargin;

		for (int i = 0; i < config_.numTrails; ++i) {
			// Pick random start and end points
			pos_t start = PickRandomPosition(
			    map.GetWidth(), map.GetHeight(), margin);
			pos_t end = PickRandomPosition(map.GetWidth(),
			                               map.GetHeight(), margin);

			// Generate the trail
			std::vector<pos_t> path =
			    GenerateTrail(map, start, end, config_.trailConfig);

			// Store trail information
			trails_.push_back(Trail { start, end, path });
		}
	}

	void BasicDungeonGenerator::ConnectTrails(Map& map)
	{
		// Collect all trail endpoints (both start and end of each
		// trail)
		std::vector<pos_t> endpoints;
		for (const auto& trail : trails_) {
			endpoints.push_back(trail.start);
			endpoints.push_back(trail.end);
		}

		// Connect each endpoint to every other endpoint
		// This ensures full connectivity (DCSS does this)
		for (size_t i = 0; i < endpoints.size(); ++i) {
			for (size_t j = i + 1; j < endpoints.size(); ++j) {
				// Find path between endpoints
				std::vector<pos_t> path =
				    FindPath(map, endpoints[i], endpoints[j]);

				// Carve the path (it should already be mostly
				// floor, but ensures connectivity)
				for (const auto& pos : path) {
					if (map.IsWall(pos)) {
						map.SetTileType(
						    pos, TileType::FLOOR);
					}
				}
			}
		}
	}

	void BasicDungeonGenerator::PlaceRooms(Map& map)
	{
		auto* rand = TCODRandom::getInstance();

		// Decide how many rooms to place
		int numRooms = rand->getInt(config_.minRooms, config_.maxRooms);

		std::vector<Room> placedRooms;

		// Try to place rooms
		for (int attempt = 0; attempt < numRooms * 10; ++attempt) {
			// Random room size
			int roomWidth = rand->getInt(config_.minRoomSize,
			                             config_.maxRoomSize);
			int roomHeight = rand->getInt(config_.minRoomSize,
			                              config_.maxRoomSize);

			// Random position (with 1-tile margin from edges)
			pos_t roomOrigin {
				rand->getInt(1, map.GetWidth() - roomWidth - 2),
				rand->getInt(1,
				             map.GetHeight() - roomHeight - 2)
			};

			Room newRoom(roomOrigin, roomWidth, roomHeight);

			// Check if it overlaps with existing rooms
			if (RoomOverlaps(newRoom, placedRooms)) {
				continue;
			}

			// Place the room (carve floor, leave walls on edges)
			for (auto pos : newRoom.GetInner()) {
				map.SetTileType(pos, TileType::FLOOR);
			}

			placedRooms.push_back(newRoom);
			map.AddRoom(newRoom);

			// Stop if we've placed enough rooms
			if (static_cast<int>(placedRooms.size()) >= numRooms) {
				break;
			}
		}
	}

} // namespace tutorial
