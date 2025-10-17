#include "TurnManager.hpp"

#include "Command.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "Event.hpp"

namespace tutorial
{
    void TurnManager::ProcessCommand(std::unique_ptr<Command> command,
                                     Engine& engine)
    {
        if (!command)
        {
            return;
        }

        // Execute the player's command
        command->Execute(engine);

        // Process any events created by the command
        engine.HandleEvents();

        // If the command consumed a turn, let enemies act
        if (command->ConsumesTurn())
        {
            ProcessEnemyTurn(engine);
        }
    }

    void TurnManager::ProcessEnemyTurn(Engine& engine)
    {
        // Queue up enemy actions
        const auto& entities = engine.GetEntities();

        for (auto& entity : entities)
        {
            if (engine.IsPlayer(*entity.get()))
            {
                continue;
            }

            if (!entity->CanAct())
            {
                continue;
            }

            std::unique_ptr<Event> event =
                std::make_unique<AiAction>(engine, *entity);
            engine.AddEventFront(event);
        }

        // Process all enemy actions
        engine.HandleEvents();
    }

} // namespace tutorial
