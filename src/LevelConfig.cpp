#include "LevelConfig.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace tutorial
{
	LevelConfig LevelConfig::LoadFromFile(const std::string& filepath)
	{
		std::ifstream file(filepath);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open level config: "
			                         + filepath);
		}

		nlohmann::json j;
		try {
			file >> j;
		} catch (const nlohmann::json::parse_error& e) {
			throw std::runtime_error("JSON parse error in "
			                         + filepath + ": " + e.what());
		}

		return FromJson(j);
	}

	LevelConfig LevelConfig::FromJson(const nlohmann::json& j)
	{
		LevelConfig config;

		// Required fields
		if (!j.contains("id")) {
			throw std::runtime_error(
			    "Level config missing required 'id' field");
		}
		config.id = j["id"];

		// Optional name/description keys
		config.nameKey = j.value("name_key", "");
		config.descriptionKey = j.value("description_key", "");

		// Parse generation parameters
		if (!j.contains("generation")) {
			throw std::runtime_error(
			    "Level config missing 'generation' section");
		}

		auto gen = j["generation"];
		config.generation.width = gen.value("width", 80);
		config.generation.height = gen.value("height", 45);
		config.generation.algorithm =
		    gen.value("algorithm", "rooms_and_corridors");

		if (!gen.contains("params")) {
			throw std::runtime_error(
			    "Level config missing 'generation.params' section");
		}

		auto params = gen["params"];
		config.generation.maxRooms = params.value("max_rooms", 30);
		config.generation.minRoomSize =
		    params.value("min_room_size", 6);
		config.generation.maxRoomSize =
		    params.value("max_room_size", 10);

		// Parse monster spawning
		if (j.contains("spawning")
		    && j["spawning"].contains("monsters")) {
			auto monsters = j["spawning"]["monsters"];
			config.monsterSpawning.maxPerRoom =
			    monsters.value("max_per_room", 3);
			config.monsterSpawning.spawnChance =
			    monsters.value("spawn_chance", 0.8f);

			if (monsters.contains("spawn_table")
			    && monsters["spawn_table"].is_array()) {
				for (const auto& entry :
				     monsters["spawn_table"]) {
					std::string id = entry["id"];
					int weight = entry["weight"];
					config.monsterSpawning.spawnTable
					    .push_back({ id, weight });
				}
			}
		}

		// Parse item spawning
		if (j.contains("spawning") && j["spawning"].contains("items")) {
			auto items = j["spawning"]["items"];
			config.itemSpawning.maxPerRoom =
			    items.value("max_per_room", 2);
			config.itemSpawning.spawnChance =
			    items.value("spawn_chance", 0.7f);

			if (items.contains("spawn_table")
			    && items["spawn_table"].is_array()) {
				for (const auto& entry : items["spawn_table"]) {
					std::string id = entry["id"];
					int weight = entry["weight"];
					config.itemSpawning.spawnTable
					    .push_back({ id, weight });
				}
			}
		}

		std::cout << "[LevelConfig] Loaded level config: " << config.id
		          << std::endl;
		std::cout << "  - Map size: " << config.generation.width << "x"
		          << config.generation.height << std::endl;
		std::cout << "  - Max rooms: " << config.generation.maxRooms
		          << std::endl;
		std::cout << "  - Monster spawn entries: "
		          << config.monsterSpawning.spawnTable.size()
		          << std::endl;
		std::cout << "  - Item spawn entries: "
		          << config.itemSpawning.spawnTable.size() << std::endl;

		return config;
	}

} // namespace tutorial
