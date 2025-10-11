#include "EntityManager.hpp"

#include "EntityFactory.hpp"
#include "Map.hpp"

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

    void EntityManager::MoveToFront(Entity& entity)
    {
        auto it = std::find_if(entities_.begin(), entities_.end(),
                               [&entity](const auto& it)
                               { return (it.get() == &entity); });

        if (it != entities_.end())
        {
            auto ent = std::move(*it);
            entities_.erase(it);
            entities_.push_front(std::move(ent));
        }
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

            std::unique_ptr<AEntityFactory> factory{ nullptr };

            // Add either an orc or a troll
            if (rand->getInt(0, 100) < 80)
            {
                factory = std::make_unique<OrcFactory>();
                Spawn(factory->Create(), pos);
            }
            else
            {
                factory = std::make_unique<TrollFactory>();
                Spawn(factory->Create(), pos);
            }
        }
    }

    std::unique_ptr<Entity>& EntityManager::Spawn(std::unique_ptr<Entity>&& src)
    {
        return entities_.emplace_back(std::move(src));
    }

    std::unique_ptr<Entity>& EntityManager::Spawn(std::unique_ptr<Entity>&& src,
                                                  pos_t pos)
    {
        auto& entity = Spawn(std::move(src));

        if (pos != entity->GetPos())
        {
            entity->SetPos(pos);
        }

        return entity;
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

                if (dice < 70)
                {
                    // 70% chance: health potion
                    HealthPotionFactory factory;
                    Spawn(factory.Create(), pos);
                }
                else if (dice < 70 + 10)
                {
                    // 10% chance: scroll of lightning bolt
                    LightningBoltFactory factory;
                    Spawn(factory.Create(), pos);
                }
                else if (dice < 70 + 10 + 10)
                {
                    // 10% chance: scroll of fireball
                    FireballFactory factory;
                    Spawn(factory.Create(), pos);
                }
                else
                {
                    // 10% chance: scroll of confusion
                    ConfuserFactory factory;
                    Spawn(factory.Create(), pos);
                }
            }
        }
    }
} // namespace tutorial
