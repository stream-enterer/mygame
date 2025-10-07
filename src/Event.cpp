#include "Event.hpp"
#include <memory>
#include <string>

#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "Util.hpp"

namespace tutorial
{
    MessageHistoryEvent::MessageHistoryEvent(Engine& engine)
        : EngineEvent(engine)
    {
    }

    void MessageHistoryEvent::Execute()
    {
        engine_.ShowMessageHistory();
    }
} // namespace tutorial

namespace tutorial
{
    NewGameEvent::NewGameEvent(Engine& engine) : EngineEvent(engine)
    {
    }

    void NewGameEvent::Execute()
    {
        engine_.NewGame();
    }
} // namespace tutorial

namespace tutorial
{
    ReturnToGameEvent::ReturnToGameEvent(Engine& engine) : EngineEvent(engine)
    {
    }

    void ReturnToGameEvent::Execute()
    {
        engine_.ReturnToMainGame();
    }
} // namespace tutorial

namespace tutorial
{
    QuitEvent::QuitEvent(Engine& engine) : EngineEvent(engine)
    {
    }

    void QuitEvent::Execute()
    {
        engine_.Quit();
    }
} // namespace tutorial

namespace tutorial
{
    void Action::Execute()
    {
        if (!engine_.IsValid(entity_))
        {
            return;
        }
    }
} // namespace tutorial

namespace tutorial
{
    AiAction::AiAction(Engine& engine, Entity& entity) : Action(engine, entity)
    {
    }

    void AiAction::Execute()
    {
        Action::Execute();

        entity_.Act(engine_);
    }
} // namespace tutorial

namespace tutorial
{
    DieAction::DieAction(Engine& engine, Entity& entity)
        : Action(engine, entity)
    {
    }

    void DieAction::Execute()
    {
        Action::Execute();

        if (engine_.IsPlayer(entity_))
        {
            engine_.LogMessage("You died!", color::dark_red, false);
        }
        else
        {
            engine_.LogMessage(
                util::capitalize(entity_.GetName()) + " has died!",
                color::dark_red, true);
        }

        engine_.HandleDeathEvent(entity_);
    }
} // namespace tutorial

namespace tutorial
{
    WaitAction::WaitAction(Engine& engine, Entity& entity)
        : Action(engine, entity)
    {
    }

    void WaitAction::Execute()
    {
        // No op
    }
} // namespace tutorial

namespace tutorial
{
    BumpAction::BumpAction(Engine& engine, Entity& entity, pos_t pos)
        : DirectionalAction(engine, entity, pos)
    {
    }

    void BumpAction::Execute()
    {
        Action::Execute();

        if (entity_.GetDestructible()->IsDead())
        {
            return;
        }

        auto targetPos = entity_.GetPos() + pos_;

        if (engine_.GetBlockingEntity(targetPos))
        {
            auto action = MeleeAction(engine_, entity_, pos_);
            std::unique_ptr<Event> event =
                std::make_unique<MeleeAction>(action);
            engine_.AddEventFront(event);
        }
        else
        {
            auto action = MoveAction(engine_, entity_, pos_);
            std::unique_ptr<Event> event = std::make_unique<MoveAction>(action);
            engine_.AddEventFront(event);
        }
    }
} // namespace tutorial

namespace tutorial
{
    MeleeAction::MeleeAction(Engine& engine, Entity& entity, pos_t pos)
        : DirectionalAction(engine, entity, pos)
    {
    }

    void MeleeAction::Execute()
    {
        Action::Execute();

        if (!entity_.GetAttacker())
        {
            return;
        }

        auto targetPos = entity_.GetPos() + pos_;
        auto* target = engine_.GetBlockingEntity(targetPos);

        if (target && !target->GetDestructible()->IsDead())
        {
            auto* attacker = entity_.GetAttacker();
            auto* defender = target->GetDestructible();
            auto damage = attacker->Attack() - defender->GetDefense();

            if (damage > 0)
            {
                defender->TakeDamage(damage);

                engine_.LogMessage(util::capitalize(entity_.GetName())
                                       + " attacks " + target->GetName()
                                       + " for " + std::to_string(damage)
                                       + " hit points.",
                                   color::red, true);

                if (defender->IsDead())
                {
                    auto action = DieAction(engine_, *target);
                    std::unique_ptr<Event> event =
                        std::make_unique<DieAction>(action);
                    engine_.AddEventFront(event);
                }
            }
            else
            {
                engine_.LogMessage(util::capitalize(entity_.GetName())
                                       + " attacks " + target->GetName()
                                       + " but does no damage.",
                                   color::red, true);
            }
        }
    }
} // namespace tutorial

namespace tutorial
{
    MoveAction::MoveAction(Engine& engine, Entity& entity, pos_t pos)
        : DirectionalAction(engine, entity, pos)
    {
    }

    void MoveAction::Execute()
    {
        Action::Execute();

        auto targetPos = entity_.GetPos() + pos_;

        if (engine_.IsInBounds(targetPos) && !engine_.IsWall(targetPos))
        {
            auto pos = entity_.GetPos() + pos_;

            entity_.SetPos(pos);

            if (engine_.IsPlayer(entity_))
            {
                engine_.ComputeFOV();
            }
        }
    }
} // namespace tutorial
