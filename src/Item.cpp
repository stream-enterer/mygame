#include "Item.hpp"

#include "Entity.hpp"

namespace tutorial
{
    void HealthPotion::Use(Entity& entity)
    {
        entity.GetDestructible()->Heal(4);
    }
} // namespace tutorial
