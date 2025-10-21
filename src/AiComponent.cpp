#include "AiComponent.hpp"

#include "Engine.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "Map.hpp"
#include "Position.hpp"
#include "StringTable.hpp"

#include <libtcod/bresenham.hpp>

#include <array>
#include <memory>
#include <numeric>

inline namespace
{
    using tutorial::pos_t;

    auto checkCardinalPoints = [](pos_t pos,
                                  pos_t target) -> tcod::BresenhamLine
    {
        constexpr std::array<pos_t, 4> cardinals{ {
            { 0, -1 }, // up
            { 0, 1 },  // down
            { -1, 0 }, // left
            { 1, 0 }   // right
        } };

        std::vector<tcod::BresenhamLine> lines;
        lines.reserve(4);

        for (auto [x, y] : cardinals)
        {
            auto new_pos = pos_t{ x, y } + target;

            auto line =
                tcod::BresenhamLine({ pos.x, pos.y }, { new_pos.x, new_pos.y })
                    .without_start();

            lines.push_back(line);
        }

        auto shortest = lines[0];

        auto bresenhamLength =
            [](const tcod::BresenhamLine& path) -> std::size_t
        {
            std::size_t length = 0;

            for (auto it = path.begin(); it != path.end(); ++it)
            {
                ++length;
            }

            return length;
        };

        for (auto line : lines)
        {
            if (bresenhamLength(shortest) > bresenhamLength(line))
            {
                shortest = line;
            }
        }

        return shortest;
    };
} // namespace

namespace tutorial
{
    void BaseAi::Perform(Engine&, Entity&)
    {
        // No op
    }

    void HostileAi::Perform(Engine& engine, Entity& entity)
    {
        auto pos = entity.GetPos();

        // Only act if entity is alive and in player's FOV
        if (entity.GetDestructible() && entity.GetDestructible()->IsDead())
        {
            return;
        }

        // If not in player's FOV, don't act (monsters only act when player can
        // see them)
        if (!engine.GetMap().IsInFov(pos))
        {
            return;
        }

        auto target = engine.GetPlayer();
        auto targetPos = target->GetPos();
        auto delta = targetPos - pos;

        auto distance = std::max(std::abs(delta.x), std::abs(delta.y));
        auto is_diagonal = (std::abs(delta.x) + std::abs(delta.y)) > 1;

        // At melee range - attack
        if (distance == 1 && !is_diagonal)
        {
            std::unique_ptr<Event> event =
                std::make_unique<MeleeAction>(engine, entity, delta);
            engine.AddEventFront(event);
            return;
        }

        // Player is visible - move directly toward them
        if (engine.GetMap().IsInFov(targetPos))
        {
            auto path = checkCardinalPoints(pos, targetPos);

            auto canPathToTarget = [](const tcod::BresenhamLine& path,
                                      const Engine& engine) -> bool
            {
                for (const auto [x, y] : path)
                {
                    if (engine.IsBlocker({ x, y }))
                    {
                        return false;
                    }
                }
                return true;
            };

            if (canPathToTarget(path, engine))
            {
                auto dest = path[0];
                auto destPos = pos_t{ dest[0], dest[1] } - pos;

                std::unique_ptr<Event> event =
                    std::make_unique<MoveAction>(engine, entity, destPos);
                engine.AddEventFront(event);
                return;
            }
        }

        // Player not visible - use scent tracking
        // Scan the 8 adjacent cells for the strongest scent
        unsigned int bestLevel = 0;
        int bestCellIndex = -1;

        // Direction offsets for 8 adjacent cells: NW, N, NE, W, E, SW, S, SE
        static constexpr int dx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        static constexpr int dy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

        for (int i = 0; i < 8; ++i)
        {
            pos_t cellPos{ pos.x + dx[i], pos.y + dy[i] };

            // Only consider walkable cells
            if (!engine.IsWall(cellPos) && !engine.IsBlocker(cellPos))
            {
                unsigned int cellScent = engine.GetMap().GetScent(cellPos);

                // Check if scent is fresh enough (not older than
                // SCENT_THRESHOLD) and better than what we've found so far
                if (cellScent > engine.GetMap().GetCurrentScentValue()
                                    - SCENT_THRESHOLD
                    && cellScent > bestLevel)
                {
                    bestLevel = cellScent;
                    bestCellIndex = i;
                }
            }
        }

        // If we found a cell with detectable scent, move toward it
        if (bestCellIndex != -1)
        {
            pos_t moveDir{ dx[bestCellIndex], dy[bestCellIndex] };
            std::unique_ptr<Event> event =
                std::make_unique<MoveAction>(engine, entity, moveDir);
            engine.AddEventFront(event);
            return;
        }

        // No valid scent trail - wait
        std::unique_ptr<Event> event =
            std::make_unique<WaitAction>(engine, entity);
        engine.AddEventFront(event);
    }

    ConfusedMonsterAi::ConfusedMonsterAi(int nbTurns,
                                         std::unique_ptr<AiComponent> oldAi) :
        nbTurns_(nbTurns), oldAi_(std::move(oldAi))
    {
    }

    void ConfusedMonsterAi::Perform(Engine& engine, Entity& entity)
    {
        // Wander randomly
        auto* rand = TCODRandom::getInstance();
        int dx = rand->getInt(-1, 1);
        int dy = rand->getInt(-1, 1);

        if (dx != 0 || dy != 0)
        {
            int destx = entity.GetPos().x + dx;
            int desty = entity.GetPos().y + dy;

            if (engine.IsInBounds(pos_t{ destx, desty })
                && !engine.IsWall(pos_t{ destx, desty }))
            {
                auto* target = engine.GetBlockingEntity(pos_t{ destx, desty });
                if (target)
                {
                    // Attack anyone including other monsters
                    std::unique_ptr<Event> event =
                        std::make_unique<MeleeAction>(engine, entity,
                                                      pos_t{ dx, dy });
                    engine.AddEventFront(event);
                }
                else
                {
                    std::unique_ptr<Event> event = std::make_unique<MoveAction>(
                        engine, entity, pos_t{ dx, dy });
                    engine.AddEventFront(event);
                }
            }
        }

        // Decrease confusion and restore old AI when done
        nbTurns_--;
        if (nbTurns_ <= 0)
        {
            auto msg = StringTable::Instance().GetMessage(
                "items.confusion_scroll.wears_off",
                { { "name", entity.GetName() } });
            engine.LogMessage(msg.text, msg.color, msg.stack);

            // Restore the original AI
            if (auto* npc = dynamic_cast<Npc*>(&entity))
            {
                npc->SwapAi(std::move(oldAi_));
            }
        }
    }
} // namespace tutorial
