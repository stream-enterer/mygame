#include "TargetSelector.hpp"

#include "Engine.hpp"
#include "Entity.hpp"
#include "StringTable.hpp"

namespace tutorial
{
    // SelfTargetSelector - targets the user
    bool SelfTargetSelector::SelectTargets(Entity& user, Engine&,
                                           std::vector<Entity*>& targets) const
    {
        targets.push_back(&user);
        return true;
    }

    // ClosestEnemySelector - finds nearest enemy in range
    ClosestEnemySelector::ClosestEnemySelector(float range) : range_(range)
    {
    }

    bool ClosestEnemySelector::SelectTargets(
        Entity& user, Engine& engine, std::vector<Entity*>& targets) const
    {
        Entity* closest =
            engine.GetClosestMonster(user.GetPos().x, user.GetPos().y, range_);

        if (closest)
        {
            targets.push_back(closest);
            return true;
        }

        auto msg = StringTable::Instance().GetMessage(
            "items.targeting.no_enemy_in_range");
        engine.LogMessage(msg.text, msg.color, msg.stack);
        return false;
    }

    // SingleTargetSelector - player picks one target
    SingleTargetSelector::SingleTargetSelector(float range) : range_(range)
    {
    }

    bool SingleTargetSelector::SelectTargets(
        Entity& /*user*/, Engine& engine, std::vector<Entity*>& targets) const
    {
        auto msg =
            StringTable::Instance().GetMessage("items.targeting.select_target");
        engine.LogMessage(msg.text, msg.color, msg.stack);

        // Close inventory to allow targeting
        engine.ReturnToMainGame();

        // Validator: checks if there's a target at the coordinates
        auto validator = [&engine](int x, int y) -> bool
        {
            Entity* target = engine.GetActor(x, y);
            if (!target)
            {
                // No target - log message and return false to stay in targeting
                auto failMsg = StringTable::Instance().GetMessage(
                    "items.targeting.no_target_at_location");
                engine.LogMessage(failMsg.text, failMsg.color, failMsg.stack);
                return false;
            }
            return true;
        };

        int x, y;
        if (!engine.PickATile(&x, &y, range_, validator))
        {
            // Player cancelled - reopen inventory
            engine.ShowInventory();
            return false;
        }

        // At this point, we know there's a valid target (validator passed)
        Entity* target = engine.GetActor(x, y);
        targets.push_back(target);
        return true;
    }

    // AreaTargetSelector - player picks tile, affects area
    AreaTargetSelector::AreaTargetSelector(float pickRange,
                                           float effectRadius) :
        pickRange_(pickRange), effectRadius_(effectRadius)
    {
    }

    bool AreaTargetSelector::SelectTargets(Entity& /*user*/, Engine& engine,
                                           std::vector<Entity*>& targets) const
    {
        auto msg =
            StringTable::Instance().GetMessage("items.targeting.select_tile");
        engine.LogMessage(msg.text, msg.color, msg.stack);

        // Close inventory to allow targeting
        engine.ReturnToMainGame();

        int x, y;
        if (!engine.PickATile(&x, &y, pickRange_))
        {
            // Player cancelled - reopen inventory
            engine.ShowInventory();
            return false;
        }

        // Find all entities in radius
        bool foundAny = false;
        for (const auto& entity : engine.GetEntities())
        {
            if (entity->GetDestructible()
                && !entity->GetDestructible()->IsDead() && !entity->IsCorpse()
                && entity->GetDistance(x, y) <= effectRadius_)
            {
                targets.push_back(entity.get());
                foundAny = true;
            }
        }

        if (!foundAny)
        {
            auto failMsg = StringTable::Instance().GetMessage(
                "items.targeting.no_targets_in_area");
            engine.LogMessage(failMsg.text, failMsg.color, failMsg.stack);
        }

        return foundAny;
    }
} // namespace tutorial
