#ifndef ENTITY_MANAGER_HPP
#define ENTITY_MANAGER_HPP

#include "Entity.hpp"
#include "Position.hpp"
#include "Room.hpp"

#include <deque>
#include <memory>

namespace tutorial
{
    class EntityManager
    {
        using Entity_ptr = std::unique_ptr<Entity>;

    public:
        void Clear();
        void MoveToFront(Entity& entity);
        void PlaceEntities(const Room& room, int maxMonstersPerRoom);
        Entity_ptr& Spawn(Entity_ptr&& src);
        Entity_ptr& Spawn(Entity_ptr&& src, pos_t pos);

        Entity* GetBlockingEntity(pos_t pos) const;

        using iterator = std::deque<Entity_ptr>::iterator;
        using const_iterator = std::deque<Entity_ptr>::const_iterator;

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

    private:
        std::deque<Entity_ptr> entities_;
    };
} // namespace tutorial

#endif // ENTITY_MANAGER_HPP
