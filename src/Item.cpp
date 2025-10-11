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

    LightningBolt::LightningBolt(float range, float damage) :
        range_(range), damage_(damage)
    {
    }

    bool LightningBolt::Use(Entity& owner, Engine& engine)
    {
        Entity* closestMonster = engine.GetClosestMonster(
            owner.GetPos().x, owner.GetPos().y, range_);

        if (!closestMonster)
        {
            engine.LogMessage("No enemy is close enough to strike.",
                              color::white, false);
            return false;
        }

        // Hit closest monster
        engine.LogMessage(
            "A lightning bolt strikes the " + closestMonster->GetName()
                + " with a loud thunder! The damage is "
                + std::to_string(static_cast<int>(damage_)) + " hit points.",
            color::light_azure, false);

        engine.DealDamage(*closestMonster, static_cast<unsigned int>(damage_));

        return true;
    }

    Fireball::Fireball(float range, float damage) :
        range_(range), damage_(damage)
    {
    }

    bool Fireball::Use(Entity& owner, Engine& engine)
    {
        engine.LogMessage(
            "Left-click a target tile for the fireball, or right-click to "
            "cancel.",
            color::light_azure, false);

        int x, y;
        if (!engine.PickATile(&x, &y, range_))
        {
            return false;
        }

        // Burn everything in range (including player)
        engine.LogMessage("The fireball explodes, burning everything within "
                              + std::to_string(static_cast<int>(range_))
                              + " tiles!",
                          color::red, false);

        for (const auto& entity : engine.GetEntities())
        {
            if (entity->GetDestructible()
                && !entity->GetDestructible()->IsDead()
                && entity->GetDistance(x, y) <= range_)
            {
                engine.LogMessage(
                    "The " + entity->GetName() + " gets burned for "
                        + std::to_string(static_cast<int>(damage_))
                        + " hit points.",
                    color::red, false);

                engine.DealDamage(*entity, static_cast<unsigned int>(damage_));
            }
        }

        return true;
    }

    Confuser::Confuser(int nbTurns, float range) :
        nbTurns_(nbTurns), range_(range)
    {
    }

    bool Confuser::Use(Entity& owner, Engine& engine)
    {
        engine.LogMessage(
            "Left-click an enemy to confuse it, or right-click to cancel.",
            color::light_azure, false);

        int x, y;
        if (!engine.PickATile(&x, &y, range_))
        {
            return false;
        }

        Entity* target = engine.GetActor(x, y);
        if (!target)
        {
            engine.LogMessage("No enemy at that location.", color::white,
                              false);
            return false;
        }

        // Only NPCs can be confused
        if (auto* npc = dynamic_cast<Npc*>(target))
        {
            engine.LogMessage(
                "The eyes of the " + target->GetName()
                    + " look vacant, as he starts to stumble around!",
                color::light_azure, false);

            // Swap in confused AI, storing the old one
            auto confusedAi = std::make_unique<ConfusedMonsterAi>(
                nbTurns_, npc->SwapAi(nullptr));
            npc->SwapAi(std::move(confusedAi));
        }
        else
        {
            engine.LogMessage("You cannot confuse that!", color::white, false);
            return false;
        }

        return true;
    }

} // namespace tutorial
