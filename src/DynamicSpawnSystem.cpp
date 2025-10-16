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

    void DynamicSpawnSystem::BuildSpawnTables()
    {
        Clear();

        // Get all entity templates
        auto allIds = TemplateRegistry::Instance().GetAllIds();

        std::cout << "[DynamicSpawnSystem] Building spawn tables from "
                  << allIds.size() << " entity templates..." << std::endl;

        for (const auto& id : allIds)
        {
            const auto* tpl = TemplateRegistry::Instance().Get(id);
            if (!tpl || tpl->spawns.empty())
            {
                continue; // Skip entities with no spawn data
            }

            // Determine if this is a monster or item
            bool isMonster = (tpl->faction == "monster");
            bool isItem = (tpl->item.has_value());

            // Add to appropriate spawn tables
            for (const auto& spawn : tpl->spawns)
            {
                if (isMonster)
                {
                    // Add to monster table for this location
                    auto& table = monsterTables_[spawn.location];
                    table.AddEntry(id, spawn.weight);

                    // Set default max monsters if not set
                    if (table.GetMaxMonstersPerRoom() == 0)
                    {
                        table.SetMaxMonstersPerRoom(3); // Default
                    }
                }
                else if (isItem)
                {
                    // Add to item table for this location
                    auto& table = itemTables_[spawn.location];
                    table.AddEntry(id, spawn.weight);

                    // Set default max items if not set
                    if (table.GetMaxItemsPerRoom() == 0)
                    {
                        table.SetMaxItemsPerRoom(2); // Default
                    }
                }
            }
        }

        std::cout << "[DynamicSpawnSystem] Built " << monsterTables_.size()
                  << " monster tables and " << itemTables_.size()
                  << " item tables" << std::endl;

        // Log what was built
        for (const auto& [location, table] : monsterTables_)
        {
            std::cout << "  - Monster table '" << location
                      << "': " << table.GetTotalWeight() << " total weight"
                      << std::endl;
        }
        for (const auto& [location, table] : itemTables_)
        {
            std::cout << "  - Item table '" << location
                      << "': " << table.GetTotalWeight() << " total weight"
                      << std::endl;
        }
    }

    const SpawnTable* DynamicSpawnSystem::GetMonsterTable(
        const std::string& location) const
    {
        auto it = monsterTables_.find(location);
        if (it != monsterTables_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const SpawnTable* DynamicSpawnSystem::GetItemTable(
        const std::string& location) const
    {
        auto it = itemTables_.find(location);
        if (it != itemTables_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool DynamicSpawnSystem::HasMonsterTable(const std::string& location) const
    {
        return monsterTables_.find(location) != monsterTables_.end();
    }

    bool DynamicSpawnSystem::HasItemTable(const std::string& location) const
    {
        return itemTables_.find(location) != itemTables_.end();
    }

    std::unordered_set<std::string> DynamicSpawnSystem::GetAllLocations() const
    {
        std::unordered_set<std::string> locations;

        for (const auto& [location, _] : monsterTables_)
        {
            locations.insert(location);
        }
        for (const auto& [location, _] : itemTables_)
        {
            locations.insert(location);
        }

        return locations;
    }

    void DynamicSpawnSystem::Clear()
    {
        monsterTables_.clear();
        itemTables_.clear();
    }

    void DynamicSpawnSystem::ValidateSpawnData() const
    {
        auto allIds = TemplateRegistry::Instance().GetAllIds();
        auto allLocations = GetAllLocations();

        std::cout << "[DynamicSpawnSystem] Validating spawn data..."
                  << std::endl;

        // Track which entities have spawn data
        int entitiesWithSpawns = 0;
        int entitiesWithoutSpawns = 0;

        for (const auto& id : allIds)
        {
            const auto* tpl = TemplateRegistry::Instance().Get(id);
            if (!tpl)
            {
                continue;
            }

            // Skip player - doesn't need spawn data
            if (tpl->faction == "player")
            {
                continue;
            }

            if (tpl->spawns.empty())
            {
                // Warn about entities without spawn data
                std::cout << "  ⚠️  WARNING: Entity '" << id
                          << "' has no spawn locations and will never appear"
                          << std::endl;
                entitiesWithoutSpawns++;
            }
            else
            {
                entitiesWithSpawns++;
            }
        }

        std::cout << "[DynamicSpawnSystem] Validation complete:" << std::endl;
        std::cout << "  - " << entitiesWithSpawns << " entities with spawn data"
                  << std::endl;
        std::cout << "  - " << entitiesWithoutSpawns
                  << " entities without spawn data (will not spawn)"
                  << std::endl;
        std::cout << "  - " << allLocations.size() << " unique spawn locations"
                  << std::endl;
    }
} // namespace tutorial
