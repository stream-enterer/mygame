#include "Item.hpp"

#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"

namespace tutorial
{
    HealthPotion::HealthPotion(unsigned int amount) : amount_(amount)
    {
    }

    bool HealthPotion::Use(Entity& owner, Engine& engine)
    {
        auto* destructible = owner.GetDestructible();
        if (destructible)
        {
            unsigned int amountHealed = destructible->Heal(amount_);
            if (amountHealed > 0)
            {
                engine.LogMessage("You use the health potion and recover "
                                      + std::to_string(amountHealed) + " HP.",
                                  color::green, false);
                return true;
            }
            else
            {
                engine.LogMessage("You are already at full health!",
                                  color::white, false);
                return false;
            }
        }
        return false;
    }
} // namespace tutorial
