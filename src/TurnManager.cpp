#include "TurnManager.hpp"

#include "Command.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "SaveManager.hpp"

namespace tutorial
{
	void TurnManager::ProcessCommand(std::unique_ptr<Command> command,
	                                 Engine& engine)
	{
		if (!command) {
			return;
		}

		// Execute the player's command
		command->Execute(engine);

		// Process any events created by the command
		engine.HandleEvents();

		// If the command consumed a turn, let enemies act
		if (command->ConsumesTurn()) {
			ProcessEnemyTurn(engine);
		}
	}

	void TurnManager::ProcessEnemyTurn(Engine& engine)
	{
		// Queue up enemy actions
		const auto& entities = engine.GetEntities();

		for (auto& entity : entities) {
			if (engine.IsPlayer(*entity.get())) {
				continue;
			}

			if (!entity->CanAct()) {
				continue;
			}

			std::unique_ptr<Event> event =
			    std::make_unique<AiAction>(engine, *entity);
			engine.AddEventFront(event);
		}

		// Process all enemy actions
		engine.HandleEvents();

		if (engine.turnsSinceLastAutosave_
		    >= Engine::kAutosaveInterval) {
			SaveManager::Instance().SaveGame(engine,
			                                 SaveType::Auto);
			// Optional: Add message to game log
			// engine.LogMessage("Game autosaved",
			// tcod::ColorRGB{100, 100, 100}, false);
			engine.turnsSinceLastAutosave_ = 0;
		}
	}

} // namespace tutorial
