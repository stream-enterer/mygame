#include "DynamicSpawnSystem.hpp"

#include "TemplateRegistry.hpp"

#include <iostream>

namespace tutorial
{
	DynamicSpawnSystem& DynamicSpawnSystem::Instance()
	{
		static DynamicSpawnSystem instance;
		return instance;
	}

	void DynamicSpawnSystem::BuildSpawnTablesForLevel(
	    const LevelConfig& level)
	{
		Clear();

		std::cout
		    << "[DynamicSpawnSystem] Building spawn tables for level: "
		    << level.id << std::endl;

		// Build monster spawn table from level config
		if (!level.monsterSpawning.spawnTable.empty()) {
			SpawnTable monsterTable;

			for (const auto& [id, weight] :
			     level.monsterSpawning.spawnTable) {
				monsterTable.AddEntry(id, weight);
			}

			monsterTable.SetMaxMonstersPerRoom(
			    level.monsterSpawning.maxPerRoom);
			monsterTables_[level.id] = monsterTable;

			std::cout << "  - Built monster table with "
			          << level.monsterSpawning.spawnTable.size()
			          << " entries, max per room: "
			          << level.monsterSpawning.maxPerRoom
			          << std::endl;
		}

		// Build item spawn table from level config
		if (!level.itemSpawning.spawnTable.empty()) {
			SpawnTable itemTable;

			for (const auto& [id, weight] :
			     level.itemSpawning.spawnTable) {
				itemTable.AddEntry(id, weight);
			}

			itemTable.SetMaxItemsPerRoom(
			    level.itemSpawning.maxPerRoom);
			itemTables_[level.id] = itemTable;

			std::cout << "  - Built item table with "
			          << level.itemSpawning.spawnTable.size()
			          << " entries, max per room: "
			          << level.itemSpawning.maxPerRoom << std::endl;
		}

		std::cout
		    << "[DynamicSpawnSystem] Spawn tables built successfully"
		    << std::endl;
	}

	const SpawnTable* DynamicSpawnSystem::GetMonsterTable(
	    const std::string& location) const
	{
		auto it = monsterTables_.find(location);
		if (it != monsterTables_.end()) {
			return &it->second;
		}
		return nullptr;
	}

	const SpawnTable* DynamicSpawnSystem::GetItemTable(
	    const std::string& location) const
	{
		auto it = itemTables_.find(location);
		if (it != itemTables_.end()) {
			return &it->second;
		}
		return nullptr;
	}

	bool DynamicSpawnSystem::HasMonsterTable(
	    const std::string& location) const
	{
		return monsterTables_.find(location) != monsterTables_.end();
	}

	bool DynamicSpawnSystem::HasItemTable(const std::string& location) const
	{
		return itemTables_.find(location) != itemTables_.end();
	}

	std::unordered_set<std::string> DynamicSpawnSystem::GetAllLocations()
	    const
	{
		std::unordered_set<std::string> locations;

		for (const auto& [location, _] : monsterTables_) {
			locations.insert(location);
		}
		for (const auto& [location, _] : itemTables_) {
			locations.insert(location);
		}

		return locations;
	}

	void DynamicSpawnSystem::Clear()
	{
		monsterTables_.clear();
		itemTables_.clear();
	}
} // namespace tutorial
