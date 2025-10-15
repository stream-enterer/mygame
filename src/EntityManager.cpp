#include "EntityManager.hpp"

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

    void EntityManager::PlaceEntities(const Room& room, int maxMonstersPerRoom)
    {
        auto* rand = TCODRandom::getInstance();
        int numMonsters = rand->getInt(0, maxMonstersPerRoom);

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

            // Spawn monster based on template
            // TODO: Better way of doing this so we don't need all of the spawn
            // weights in this file
            std::string templateId;
            if (rand->getInt(0, 100) < 80)
            {
                templateId = "orc";
            }
            else
            {
                templateId = "troll";
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
    void EntityManager::PlaceItems(const Room& room, int maxItemsPerRoom)
    {
        auto* rand = TCODRandom::getInstance();
        int numItems = rand->getInt(0, maxItemsPerRoom);

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
                // Roll dice to determine item type
                int dice = rand->getInt(0, 100);

                std::string itemTemplateId;

                if (dice < 70)
                {
                    // 70% chance: health potion
                    itemTemplateId = "health_potion";
                }
                else if (dice < 70 + 10)
                {
                    // 10% chance: scroll of lightning bolt
                    itemTemplateId = "lightning_scroll";
                }
                else if (dice < 70 + 10 + 10)
                {
                    // 10% chance: scroll of fireball
                    itemTemplateId = "fireball_scroll";
                }
                else
                {
                    // 10% chance: scroll of confusion
                    itemTemplateId = "confusion_scroll";
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
