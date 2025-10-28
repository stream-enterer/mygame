#include "Command.hpp"

#include "Engine.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "InventoryMode.hpp"
#include "Position.hpp"
#include "SaveManager.hpp"

namespace tutorial
{
	// UI Commands (don't consume turns)

	void OpenInventoryCommand::Execute(Engine& engine)
	{
		engine.SetInventoryMode(InventoryMode::Use);
		engine.ShowInventory();
	}

	void OpenDropInventoryCommand::Execute(Engine& engine)
	{
		engine.SetInventoryMode(InventoryMode::Drop);
		engine.ShowInventory();
	}

	void OpenMessageHistoryCommand::Execute(Engine& engine)
	{
		engine.ShowMessageHistory();
	}

	void CloseUICommand::Execute(Engine& engine)
	{
		engine.ReturnToMainGame();
	}

	void StartMenuCommand::Execute(Engine& engine)
	{
		engine.ShowStartMenu();
	}

	void QuitCommand::Execute(Engine& engine)
	{
		// Save game unless it's game over
		if (!engine.IsGameOver()) {
			SaveManager::Instance().SaveGame(engine,
			                                 SaveType::Manual);
		}

		engine.Quit();
	}

	void OpenPauseMenuCommand::Execute(Engine& engine)
	{
		engine.ShowPauseMenu();
	}

	// Gameplay Commands (consume turns)

	void MoveCommand::Execute(Engine& engine)
	{
		auto targetPos =
		    engine.GetPlayer()->GetPos() + pos_t { dx_, dy_ };

		// Don't create action if moving into wall or out of bounds
		if (engine.IsWall(targetPos) || !engine.IsInBounds(targetPos)) {
			consumesTurn_ = false;
			return;
		}

		auto event = std::make_unique<BumpAction>(
		    engine, *engine.GetPlayer(), pos_t { dx_, dy_ });
		event->Execute();
	}

	bool MoveCommand::ConsumesTurn()
	{
		return consumesTurn_;
	}

	void WaitCommand::Execute(Engine& engine)
	{
		auto event =
		    std::make_unique<WaitAction>(engine, *engine.GetPlayer());
		event->Execute();
	}

	void PickupCommand::Execute(Engine& engine)
	{
		auto event =
		    std::make_unique<PickupAction>(engine, *engine.GetPlayer());
		event->Execute();
	}

	void DescendStairsCommand::Execute(Engine& engine)
	{
		auto* stairs = engine.GetStairs();
		if (!stairs) {
			return;
		}

		auto playerPos = engine.GetPlayer()->GetPos();
		auto stairsPos = stairs->GetPos();

		if (playerPos == stairsPos) {
			engine.NextLevel();
		} else {
			engine.LogMessage("There are no stairs here.",
			                  tcod::ColorRGB { 255, 255, 0 },
			                  false);
		}
	}

	void PickupItemCommand::Execute(Engine& engine)
	{
		if (!item_) {
			return;
		}

		auto event = std::make_unique<PickupItemAction>(
		    engine, *engine.GetPlayer(), item_);
		event->Execute();
	}

	void UseItemCommand::Execute(Engine& engine)
	{
		auto event = std::make_unique<UseItemAction>(
		    engine, *engine.GetPlayer(), itemIndex_);
		event->Execute();

		// Check if item was actually used (might have failed)
		// For now, assume it always succeeds
		consumedTurn_ = true;
	}

	void DropItemCommand::Execute(Engine& engine)
	{
		auto event = std::make_unique<DropItemAction>(
		    engine, *engine.GetPlayer(), itemIndex_);
		event->Execute();
	}

} // namespace tutorial
