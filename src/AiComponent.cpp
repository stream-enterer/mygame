#include "AiComponent.hpp"

#include "Engine.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "Position.hpp"

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
} // namespace tutorial

namespace tutorial
{
    void HostileAi::Perform(Engine& engine, Entity& entity)
    {
        const auto pos = entity.GetPos();

        if (!engine.IsInFov(pos))
        {
            return;
        }

        auto target = engine.GetPlayer();
        auto targetPos = target->GetPos();
        auto delta = targetPos - pos;

        auto distance = std::max(std::abs(delta.x), std::abs(delta.y));
        auto is_diagonal = (std::abs(delta.x) + std::abs(delta.y)) > 1;

        if (distance == 1 && !is_diagonal)
        {
            auto action = MeleeAction(engine, entity, delta);
            std::unique_ptr<Event> event =
                std::make_unique<MeleeAction>(action);
            engine.AddEventFront(event);

            return;
        }

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

            auto action = MoveAction(engine, entity, destPos);
            std::unique_ptr<Event> event = std::make_unique<MoveAction>(action);
            engine.AddEventFront(event);

            return;
        }

        auto action = WaitAction(engine, entity);
        std::unique_ptr<Event> event = std::make_unique<WaitAction>(action);
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
                    auto action = MeleeAction(engine, entity, pos_t{ dx, dy });
                    std::unique_ptr<Event> event =
                        std::make_unique<MeleeAction>(action);
                    engine.AddEventFront(event);
                }
                else
                {
                    auto action = MoveAction(engine, entity, pos_t{ dx, dy });
                    std::unique_ptr<Event> event =
                        std::make_unique<MoveAction>(action);
                    engine.AddEventFront(event);
                }
            }
        }

        // Decrease confusion and restore old AI when done
        nbTurns_--;
        if (nbTurns_ <= 0)
        {
            engine.LogMessage(
                "The " + entity.GetName() + " is no longer confused!",
                color::red, false);

            // Restore the original AI
            if (auto* npc = dynamic_cast<Npc*>(&entity))
            {
                npc->SwapAi(std::move(oldAi_));
            }
        }
    }
} // namespace tutorial
