#ifndef LEVEL_CONFIG_HPP
#define LEVEL_CONFIG_HPP

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace tutorial
{
	// Configuration for entity spawning (monsters or items)
	struct SpawnConfig {
		int maxPerRoom;
		float spawnChance;
		std::vector<std::pair<std::string, int>>
		    spawnTable; // (entity_id, weight)

		SpawnConfig() : maxPerRoom(0), spawnChance(0.0f)
		{
		}
	};

	// Complete level configuration loaded from JSON
	struct LevelConfig {
		std::string id;
		std::string nameKey;
		std::string descriptionKey;

		// Map generation parameters
		struct {
			int width;
			int height;
			std::string algorithm;
			int maxRooms;
			int minRoomSize;
			int maxRoomSize;
		} generation;

		// Monster spawning configuration
		SpawnConfig monsterSpawning;

		// Item spawning configuration
		SpawnConfig itemSpawning;

		// Load a level config from a JSON file
		static LevelConfig LoadFromFile(const std::string& filepath);

		// Load from JSON object (for when JSON is already parsed)
		static LevelConfig FromJson(const nlohmann::json& j);
	};
} // namespace tutorial

#endif // LEVEL_CONFIG_HPP
