#ifndef DYNAMIC_SPAWN_SYSTEM_HPP
#define DYNAMIC_SPAWN_SYSTEM_HPP

#include "LevelConfig.hpp"
#include "SpawnTable.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace tutorial
{
	class DynamicSpawnSystem
	{
	public:
		// Get singleton instance
		static DynamicSpawnSystem& Instance();

		// Build spawn tables from a level configuration
		void BuildSpawnTablesForLevel(const LevelConfig& level);

		// Get spawn table for a location
		const SpawnTable* GetMonsterTable(
		    const std::string& location) const;
		const SpawnTable* GetItemTable(
		    const std::string& location) const;

		// Check if location has any spawns
		bool HasMonsterTable(const std::string& location) const;
		bool HasItemTable(const std::string& location) const;

		// Get all known locations
		std::unordered_set<std::string> GetAllLocations() const;

		// Clear all tables
		void Clear();

	private:
		DynamicSpawnSystem() = default;
		~DynamicSpawnSystem() = default;

		// Prevent copying
		DynamicSpawnSystem(const DynamicSpawnSystem&) = delete;
		DynamicSpawnSystem& operator=(const DynamicSpawnSystem&) =
		    delete;

		std::unordered_map<std::string, SpawnTable> monsterTables_;
		std::unordered_map<std::string, SpawnTable> itemTables_;
	};
} // namespace tutorial

#endif // DYNAMIC_SPAWN_SYSTEM_HPP
