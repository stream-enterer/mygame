#include "EntityManager.hpp"

#include "DynamicSpawnSystem.hpp"
#include "LevelConfig.hpp"
#include "Map.hpp"
#include "RenderLayer.hpp"
#include "TemplateRegistry.hpp"

#include <iostream>
#include <libtcod/mersenne.hpp>
#include <memory>

namespace tutorial
{
    void EntityManager::Clear()
    {
        entities_.clear();
    }

    Entity* EntityManager::GetBlockingEntity(pos_t pos) const
    {
        for (auto& entity : entities_)
        {
            if (pos == entity->GetPos() && entity->IsBlocker())
            {
                return entity.get();
            }
        }

        return nullptr;
    }

    void EntityManager::SortByRenderLayer()
    {
        // Sort entities by render layer (lower layers render first/bottom)
        std::sort(entities_.begin(), entities_.end(),
                  [](const Entity_ptr& a, const Entity_ptr& b)
                  {
                      RenderLayer layerA = a->GetRenderLayer();
                      RenderLayer layerB = b->GetRenderLayer();
                      return static_cast<int>(layerA)
                             < static_cast<int>(layerB);
                  });
    }

    void EntityManager::PlaceEntities(const Room& room,
                                      const SpawnConfig& spawnConfig,
                                      const std::string& levelId)
    {
        auto* rand = TCODRandom::getInstance();

        // Check spawn chance - should we spawn monsters in this room at all?
        if (rand->getFloat(0.0f, 1.0f) > spawnConfig.spawnChance)
        {
            return; // Skip spawning in this room
        }

        // Get the spawn table for this level
        const SpawnTable* monsterTable =
            DynamicSpawnSystem::Instance().GetMonsterTable(levelId);

        if (!monsterTable)
        {
            std::cerr << "[EntityManager] No monster spawn table found for "
                      << levelId << std::endl;
            return;
        }

        // Get max monsters from spawn config
        int maxMonsters = spawnConfig.maxPerRoom;
        int numMonsters = rand->getInt(0, maxMonsters);

        for (int i = 0; i < numMonsters; ++i)
        {
            auto origin = room.GetOrigin();
            auto end = room.GetEnd();
            int x = rand->getInt(origin.x + 1, end.x - 1);
            int y = rand->getInt(origin.y + 1, end.y - 1);
            pos_t pos{ x, y };

            // Check if a monster already exists
            if (GetBlockingEntity(pos))
            {
                continue;
            }

            // Roll on spawn table to get monster type
            std::string templateId = monsterTable->Roll();
            if (templateId.empty())
            {
                continue;
            }

            auto entity = TemplateRegistry::Instance().Create(templateId, pos);
            Spawn(std::move(entity));
        }
    }

    std::unique_ptr<Entity>& EntityManager::Spawn(std::unique_ptr<Entity>&& src)
    {
        auto& entity = entities_.emplace_back(std::move(src));
        // Sort after spawning to maintain proper render order
        SortByRenderLayer();
        return entity;
    }

    std::unique_ptr<Entity>& EntityManager::Spawn(std::unique_ptr<Entity>&& src,
                                                  pos_t pos)
    {
        src->SetPos(pos);
        auto& entity = entities_.emplace_back(std::move(src));
        // Sort after spawning to maintain proper render order
        SortByRenderLayer();
        return entity;
    }

    std::unique_ptr<Entity>& EntityManager::SpawnAtFront(
        std::unique_ptr<Entity>&& src, pos_t pos)
    {
        src->SetPos(pos);
        entities_.push_front(std::move(src));
        // Sort after spawning to maintain proper render order
        SortByRenderLayer();
        return entities_.front();
    }

    std::unique_ptr<Entity> EntityManager::Remove(Entity* entity)
    {
        for (auto it = entities_.begin(); it != entities_.end(); ++it)
        {
            if (it->get() == entity)
            {
                auto removed = std::move(*it);
                entities_.erase(it);
                return removed;
            }
        }
        return nullptr;
    }
} // namespace tutorial

namespace tutorial
{
    void EntityManager::PlaceItems(const Room& room,
                                   const SpawnConfig& spawnConfig,
                                   const std::string& levelId)
    {
        auto* rand = TCODRandom::getInstance();

        // Check spawn chance - should we spawn items in this room at all?
        if (rand->getFloat(0.0f, 1.0f) > spawnConfig.spawnChance)
        {
            return; // Skip spawning in this room
        }

        // Get the spawn table for this level
        const SpawnTable* itemTable =
            DynamicSpawnSystem::Instance().GetItemTable(levelId);

        if (!itemTable)
        {
            std::cerr << "[EntityManager] No item spawn table found for "
                      << levelId << std::endl;
            return;
        }

        // Get max items from spawn config
        int maxItems = spawnConfig.maxPerRoom;
        int numItems = rand->getInt(0, maxItems);

        for (int i = 0; i < numItems; ++i)
        {
            auto origin = room.GetOrigin();
            auto end = room.GetEnd();
            int x = rand->getInt(origin.x + 1, end.x - 1);
            int y = rand->getInt(origin.y + 1, end.y - 1);
            pos_t pos{ x, y };

            // Check if something already exists at this position
            bool blocked = false;
            for (const auto& entity : entities_)
            {
                if (entity->GetPos() == pos)
                {
                    blocked = true;
                    break;
                }
            }

            if (!blocked)
            {
                // Roll on spawn table to get item type
                std::string itemTemplateId = itemTable->Roll();
                if (itemTemplateId.empty())
                {
                    continue;
                }

                auto item =
                    TemplateRegistry::Instance().Create(itemTemplateId, pos);
                std::cout << "[EntityManager] Spawning item: " << itemTemplateId
                          << " at (" << pos.x << ", " << pos.y << ")"
                          << std::endl;
                Spawn(std::move(item));
            }
        }
    }
} // namespace tutorial
