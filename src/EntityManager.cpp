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
		for (auto& entity : entities_) {
			if (pos == entity->GetPos() && entity->IsBlocker()) {
				return entity.get();
			}
		}

		return nullptr;
	}

	void EntityManager::SortByRenderLayer()
	{
		// Stable sort by (layer, priority) - preserves insertion order
		// for equal elements
		std::stable_sort(
		    entities_.begin(), entities_.end(),
		    [](const Entity_ptr& a, const Entity_ptr& b) {
			    RenderLayer layerA = a->GetRenderLayer();
			    RenderLayer layerB = b->GetRenderLayer();

			    // First compare by layer
			    if (layerA != layerB) {
				    return static_cast<int>(layerA)
				           < static_cast<int>(layerB);
			    }

			    // Within same layer, compare by render priority
			    return a->GetRenderPriority()
			           < b->GetRenderPriority();
		    });
	}

	void EntityManager::PlaceEntitiesFromTable(
	    const Room& room, const SpawnTable* table,
	    const SpawnConfig& spawnConfig, bool checkBlockingOnly)
	{
		auto* rand = TCODRandom::getInstance();

		if (rand->getFloat(0.0f, 1.0f) > spawnConfig.spawnChance) {
			return;
		}

		if (!table) {
			return;
		}

		int maxEntities = spawnConfig.maxPerRoom;
		int numEntities = rand->getInt(0, maxEntities);

		for (int i = 0; i < numEntities; ++i) {
			auto origin = room.GetOrigin();
			auto end = room.GetEnd();
			int x = rand->getInt(origin.x + 1, end.x - 1);
			int y = rand->getInt(origin.y + 1, end.y - 1);
			pos_t pos { x, y };

			bool blocked = false;
			if (checkBlockingOnly) {
				blocked = (GetBlockingEntity(pos) != nullptr);
			} else {
				for (const auto& entity : entities_) {
					if (entity->GetPos() == pos) {
						blocked = true;
						break;
					}
				}
			}

			if (blocked) {
				continue;
			}

			std::string templateId = table->Roll();
			if (templateId.empty()) {
				continue;
			}

			auto entity =
			    TemplateRegistry::Instance().Create(templateId, pos);
			Spawn(std::move(entity));
		}
	}

	void EntityManager::PlaceEntities(const Room& room,
	                                  const SpawnConfig& spawnConfig,
	                                  const std::string& levelId)
	{
		const SpawnTable* monsterTable =
		    DynamicSpawnSystem::Instance().GetMonsterTable(levelId);

		if (!monsterTable) {
			std::cerr << "[EntityManager] No monster spawn table "
			             "found for "
			          << levelId << std::endl;
			return;
		}

		PlaceEntitiesFromTable(room, monsterTable, spawnConfig, true);
	}

	std::unique_ptr<Entity>& EntityManager::Spawn(
	    std::unique_ptr<Entity>&& src)
	{
		auto& entity = entities_.emplace_back(std::move(src));
		SortByRenderLayer();
		return entity;
	}

	std::unique_ptr<Entity>& EntityManager::Spawn(
	    std::unique_ptr<Entity>&& src, pos_t pos)
	{
		src->SetPos(pos);
		auto& entity = entities_.emplace_back(std::move(src));
		SortByRenderLayer();
		return entity;
	}

	std::unique_ptr<Entity> EntityManager::Remove(Entity* entity)
	{
		for (auto it = entities_.begin(); it != entities_.end(); ++it) {
			if (it->get() == entity) {
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
		const SpawnTable* itemTable =
		    DynamicSpawnSystem::Instance().GetItemTable(levelId);

		if (!itemTable) {
			std::cerr
			    << "[EntityManager] No item spawn table found for "
			    << levelId << std::endl;
			return;
		}

		PlaceEntitiesFromTable(room, itemTable, spawnConfig, false);
	}
} // namespace tutorial
