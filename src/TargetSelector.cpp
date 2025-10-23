#include "TargetSelector.hpp"

#include "Engine.hpp"
#include "Entity.hpp"
#include "Map.hpp"
#include "StringTable.hpp"

#include <libtcod/bresenham.hpp>

namespace tutorial
{
    // Helper: Check line-of-sight using Bresenham's line algorithm
    bool TargetSelector::HasLineOfSight(Engine& engine, pos_t origin,
                                        pos_t target) const
    {
        const Map& map = engine.GetMap();

        // Use libtcod's Bresenham line iterator
        tcod::BresenhamLine line({ origin.x, origin.y },
                                 { target.x, target.y });

        // Check each tile along the line (excluding endpoints)
        for (auto it = line.begin(); it != line.end(); ++it)
        {
            auto [x, y] = *it;

            // Skip the origin and target tiles
            if ((x == origin.x && y == origin.y)
                || (x == target.x && y == target.y))
            {
                continue;
            }

            // If we hit a non-transparent tile, LOS is blocked
            if (!map.IsTransparent(pos_t{ x, y }))
            {
                return false;
            }
        }

        return true;
    }

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
        Entity& user, Engine& engine, std::vector<Entity*>& targets) const
    {
        auto msg =
            StringTable::Instance().GetMessage("items.targeting.select_target");
        engine.LogMessage(msg.text, msg.color, msg.stack);

        // Close inventory to allow targeting
        engine.ReturnToMainGame();

        // Validator: checks if there's a target at the coordinates AND
        // line-of-sight
        auto validator = [&engine, &user, this](int x, int y) -> bool
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

            // Check line-of-sight
            if (!HasLineOfSight(engine, user.GetPos(), pos_t{ x, y }))
            {
                auto losMsg = StringTable::Instance().GetMessage(
                    "items.targeting.no_line_of_sight");
                engine.LogMessage(losMsg.text, losMsg.color, losMsg.stack);
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

    bool AreaTargetSelector::SelectTargets(Entity& user, Engine& engine,
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

        // Find all entities in radius with line-of-sight
        bool foundAny = false;
        for (const auto& entity : engine.GetEntities())
        {
            if (entity->GetDestructible()
                && !entity->GetDestructible()->IsDead() && !entity->IsCorpse()
                && entity->GetDistance(x, y) <= effectRadius_
                && HasLineOfSight(engine, user.GetPos(), entity->GetPos()))
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

    // BeamTargetSelector - player picks direction, affects line
    BeamTargetSelector::BeamTargetSelector(float range) : range_(range)
    {
    }

    bool BeamTargetSelector::SelectTargets(Entity& user, Engine& engine,
                                           std::vector<Entity*>& targets) const
    {
        auto msg =
            StringTable::Instance().GetMessage("items.targeting.select_target");
        engine.LogMessage(msg.text, msg.color, msg.stack);

        // Close inventory to allow targeting
        engine.ReturnToMainGame();

        int x, y;
        if (!engine.PickATile(&x, &y, range_))
        {
            // Player cancelled - reopen inventory
            engine.ShowInventory();
            return false;
        }

        // Trace beam from user to selected tile using Bresenham
        const Map& map = engine.GetMap();
        pos_t userPos = user.GetPos();
        tcod::BresenhamLine line({ userPos.x, userPos.y }, { x, y });

        // Collect all tiles the beam passes through
        std::vector<pos_t> beamTiles;
        for (auto it = line.begin(); it != line.end(); ++it)
        {
            auto [bx, by] = *it;
            pos_t tilePos{ bx, by };

            // Skip the user's tile
            if (bx == userPos.x && by == userPos.y)
            {
                continue;
            }

            // Stop beam at walls
            if (!map.IsTransparent(tilePos))
            {
                break;
            }

            // Stop beam at max range
            if (user.GetDistance(bx, by) > range_)
            {
                break;
            }

            beamTiles.push_back(tilePos);
        }

        // Find all entities on the beam path
        bool foundAny = false;
        for (const pos_t& tilePos : beamTiles)
        {
            for (const auto& entity : engine.GetEntities())
            {
                if (entity->GetDestructible()
                    && !entity->GetDestructible()->IsDead()
                    && !entity->IsCorpse() && entity->GetPos().x == tilePos.x
                    && entity->GetPos().y == tilePos.y)
                {
                    targets.push_back(entity.get());
                    foundAny = true;
                }
            }
        }

        if (!foundAny)
        {
            auto failMsg = StringTable::Instance().GetMessage(
                "items.targeting.no_targets_in_beam");
            engine.LogMessage(failMsg.text, failMsg.color, failMsg.stack);
        }

        return foundAny;
    }
} // namespace tutorial
