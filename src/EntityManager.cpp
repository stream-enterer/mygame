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

	void EntityManager::PlaceEntities(const Room& room,
	                                  const SpawnConfig& spawnConfig,
	                                  const std::string& levelId)
	{
		auto* rand = TCODRandom::getInstance();

		if (rand->getFloat(0.0f, 1.0f) > spawnConfig.spawnChance) {
			return;
		}

		const SpawnTable* monsterTable =
		    DynamicSpawnSystem::Instance().GetMonsterTable(levelId);

		if (!monsterTable) {
			std::cerr << "[EntityManager] No monster spawn table "
			             "found for "
			          << levelId << std::endl;
			return;
		}

		int maxMonsters = spawnConfig.maxPerRoom;
		int numMonsters = rand->getInt(0, maxMonsters);

		for (int i = 0; i < numMonsters; ++i) {
			auto origin = room.GetOrigin();
			auto end = room.GetEnd();
			int x = rand->getInt(origin.x + 1, end.x - 1);
			int y = rand->getInt(origin.y + 1, end.y - 1);
			pos_t pos { x, y };

			if (GetBlockingEntity(pos)) {
				continue;
			}

			std::string templateId = monsterTable->Roll();
			if (templateId.empty()) {
				continue;
			}

			auto entity = TemplateRegistry::Instance().Create(
			    templateId, pos);
			Spawn(std::move(entity));
		}
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
		auto* rand = TCODRandom::getInstance();

		if (rand->getFloat(0.0f, 1.0f) > spawnConfig.spawnChance) {
			return;
		}

		const SpawnTable* itemTable =
		    DynamicSpawnSystem::Instance().GetItemTable(levelId);

		if (!itemTable) {
			std::cerr
			    << "[EntityManager] No item spawn table found for "
			    << levelId << std::endl;
			return;
		}

		int maxItems = spawnConfig.maxPerRoom;
		int numItems = rand->getInt(0, maxItems);

		for (int i = 0; i < numItems; ++i) {
			auto origin = room.GetOrigin();
			auto end = room.GetEnd();
			int x = rand->getInt(origin.x + 1, end.x - 1);
			int y = rand->getInt(origin.y + 1, end.y - 1);
			pos_t pos { x, y };

			bool blocked = false;
			for (const auto& entity : entities_) {
				if (entity->GetPos() == pos) {
					blocked = true;
					break;
				}
			}

			if (!blocked) {
				std::string itemTemplateId = itemTable->Roll();
				if (itemTemplateId.empty()) {
					continue;
				}

				auto item = TemplateRegistry::Instance().Create(
				    itemTemplateId, pos);
				std::cout << "[EntityManager] Spawning item: "
				          << itemTemplateId << " at (" << pos.x
				          << ", " << pos.y << ")" << std::endl;
				Spawn(std::move(item));
			}
		}
	}
} // namespace tutorial
