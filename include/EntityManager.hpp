#ifndef ENTITY_MANAGER_HPP
#define ENTITY_MANAGER_HPP

#include "Entity.hpp"
#include "Position.hpp"
#include "Room.hpp"

namespace tutorial
{
	struct SpawnConfig;
}

#include <deque>
#include <memory>

namespace tutorial
{
	class EntityManager
	{
		using Entity_ptr = std::unique_ptr<Entity>;

	public:
		void Clear();
		void PlaceEntities(const Room& room,
		                   const SpawnConfig& spawnConfig,
		                   const std::string& levelId);
		void PlaceItems(const Room& room,
		                const SpawnConfig& spawnConfig,
		                const std::string& levelId);
		void SortByRenderLayer();
		Entity_ptr& Spawn(Entity_ptr&& src);
		Entity_ptr& Spawn(Entity_ptr&& src, pos_t pos);

		Entity* GetBlockingEntity(pos_t pos) const;
		std::unique_ptr<Entity> Remove(Entity* entity);

		using iterator = std::deque<Entity_ptr>::iterator;
		using const_iterator = std::deque<Entity_ptr>::const_iterator;
		using reverse_iterator =
		    std::deque<Entity_ptr>::reverse_iterator;
		using const_reverse_iterator =
		    std::deque<Entity_ptr>::const_reverse_iterator;

		iterator begin()
		{
			return entities_.begin();
		}

		iterator end()
		{
			return entities_.end();
		}

		const_iterator begin() const
		{
			return entities_.begin();
		}

		const_iterator end() const
		{
			return entities_.end();
		}

		// ADD THESE FOUR METHODS:
		reverse_iterator rbegin()
		{
			return entities_.rbegin();
		}

		reverse_iterator rend()
		{
			return entities_.rend();
		}

		const_reverse_iterator rbegin() const
		{
			return entities_.rbegin();
		}

		const_reverse_iterator rend() const
		{
			return entities_.rend();
		}

	private:
		void PlaceEntitiesFromTable(
		    const Room& room, const class SpawnTable* table,
		    const SpawnConfig& spawnConfig,
		    bool checkBlockingOnly);

		std::deque<Entity_ptr> entities_;
	};
} // namespace tutorial

#endif // ENTITY_MANAGER_HPP
