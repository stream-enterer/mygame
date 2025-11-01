#ifndef BASIC_DUNGEON_GENERATOR_HPP
#define BASIC_DUNGEON_GENERATOR_HPP

#include "MapGenerator.hpp"
#include "Position.hpp"
#include "TrailGenerator.hpp"

#include <vector>

namespace tutorial
{
	// Configuration for basic dungeon generation
	struct BasicDungeonConfig {
		int numTrails;   // Number of winding trails (DCSS uses 3)
		int minRooms;    // Minimum number of rooms
		int maxRooms;    // Maximum number of rooms
		int minRoomSize; // Minimum room dimension
		int maxRoomSize; // Maximum room dimension
		TrailConfig trailConfig; // Trail generation parameters
	};

	// Stores information about a generated trail
	struct Trail {
		pos_t start;             // Starting position
		pos_t end;               // Ending position
		std::vector<pos_t> path; // All carved positions
	};

	// DCSS-style basic dungeon generator
	// Generates: trails -> connections -> rooms -> doors
	class BasicDungeonGenerator : public Map::Generator
	{
	public:
		explicit BasicDungeonGenerator(
		    const BasicDungeonConfig& config);

		// Main generation function (implements Map::Generator
		// interface)
		void Generate(Map& map) override;

		// Get default DCSS-like configuration
		static BasicDungeonConfig GetDefaultConfig(int width,
		                                           int height);

	private:
		// Phase 1: Generate winding trails
		void GenerateTrails(Map& map);

		// Phase 2: Connect all trail endpoints with pathfinding
		void ConnectTrails(Map& map);

		// Phase 3: Place rooms over the structure
		void PlaceRooms(Map& map);

		BasicDungeonConfig config_;
		std::vector<Trail> trails_;
	};

} // namespace tutorial

#endif // BASIC_DUNGEON_GENERATOR_HPP
