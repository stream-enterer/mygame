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

	void NewGameCommand::Execute(Engine& engine)
	{
		engine.NewGame();
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

	void MenuNavigateUpCommand::Execute(Engine& engine)
	{
		engine.MenuNavigateUp();
	}

	void MenuNavigateDownCommand::Execute(Engine& engine)
	{
		engine.MenuNavigateDown();
	}

	void MenuConfirmCommand::Execute(Engine& engine)
	{
		engine.MenuConfirm();
	}

	void MenuNavigateLeftCommand::Execute(Engine& engine)
	{
		engine.MenuNavigateLeft();
	}

	void MenuNavigateRightCommand::Execute(Engine& engine)
	{
		engine.MenuNavigateRight();
	}

	void MenuSelectLetterCommand::Execute(Engine& engine)
	{
		engine.MenuSelectByLetter(letter_);
	}

	void MenuIncrementStatCommand::Execute(Engine& engine)
	{
		engine.MenuIncrementStat();
	}

	void MenuDecrementStatCommand::Execute(Engine& engine)
	{
		engine.MenuDecrementStat();
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

		consumesTurn_ = true;
		std::unique_ptr<Event> action = std::make_unique<BumpAction>(
		    engine, *engine.GetPlayer(), pos_t { dx_, dy_ });
		engine.AddEventFront(action);
	}

	bool MoveCommand::ConsumesTurn()
	{
		return consumesTurn_;
	}

	void WaitCommand::Execute(Engine& engine)
	{
		std::unique_ptr<Event> action =
		    std::make_unique<WaitAction>(engine, *engine.GetPlayer());
		engine.AddEventFront(action);
	}

	void PickupCommand::Execute(Engine& engine)
	{
		std::unique_ptr<Event> action =
		    std::make_unique<PickupAction>(engine, *engine.GetPlayer());
		engine.AddEventFront(action);
	}

	void DescendStairsCommand::Execute(Engine& engine)
	{
		// Check if player is on the stairs
		Entity* stairs = engine.GetStairs();
		Entity* player = engine.GetPlayer();

		if (!stairs || !player) {
			return;
		}

		if (player->GetPos() == stairs->GetPos()) {
			// Player is on stairs - descend to next level
			engine.NextLevel();
		} else {
			// Not on stairs - display message
			engine.LogMessage("There are no stairs here.",
			                  { 128, 128, 128 }, false);
		}
	}

	void PickupItemCommand::Execute(Engine& engine)
	{
		std::unique_ptr<Event> action =
		    std::make_unique<PickupItemAction>(
		        engine, *engine.GetPlayer(), item_);
		engine.AddEventFront(action);
	}

	void UseItemCommand::Execute(Engine& engine)
	{
		consumedTurn_ = false; // Default to not consuming

		if (auto* player = dynamic_cast<Player*>(engine.GetPlayer())) {
			if (Entity* item =
			        player->GetInventoryItem(itemIndex_)) {
				if (item->GetItem()) {
					// Execute item use directly to get
					// return value
					bool itemUsed = item->GetItem()->Use(
					    *player, engine);

					if (itemUsed) {
						// Item was consumed
						player->RemoveFromInventory(
						    itemIndex_);
						consumedTurn_ = true;
					}
					// else: item use failed/canceled, turn
					// not consumed
				}
			}
		}
	}

	void DropItemCommand::Execute(Engine& engine)
	{
		std::unique_ptr<Event> action =
		    std::make_unique<DropItemAction>(
		        engine, *engine.GetPlayer(), itemIndex_);
		engine.AddEventFront(action);
	}

} // namespace tutorial
