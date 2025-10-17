#include "Command.hpp"

#include "Engine.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "InventoryMode.hpp"
#include "Position.hpp"

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

    void NewGameCommand::Execute(Engine& engine)
    {
        engine.NewGame();
    }

    void QuitCommand::Execute(Engine& engine)
    {
        engine.Quit();
    }

    // Gameplay Commands (consume turns)

    void MoveCommand::Execute(Engine& engine)
    {
        std::unique_ptr<Event> action = std::make_unique<BumpAction>(
            engine, *engine.GetPlayer(), pos_t{ dx_, dy_ });
        engine.AddEventFront(action);
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

    void UseItemCommand::Execute(Engine& engine)
    {
        std::unique_ptr<Event> action = std::make_unique<UseItemAction>(
            engine, *engine.GetPlayer(), itemIndex_);
        engine.AddEventFront(action);
    }

    void DropItemCommand::Execute(Engine& engine)
    {
        std::unique_ptr<Event> action = std::make_unique<DropItemAction>(
            engine, *engine.GetPlayer(), itemIndex_);
        engine.AddEventFront(action);
    }

} // namespace tutorial
