#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include "Command.hpp"
#include "Event.hpp"
#include "InventoryMode.hpp"
#include "KeyPress.hpp"

#include <memory>
#include <unordered_map>

namespace tutorial
{
	enum Actions {
		MOVE_UP = 0,
		MOVE_DOWN,
		MOVE_LEFT,
		MOVE_RIGHT,
		MOVE_UP_LEFT,
		MOVE_UP_RIGHT,
		MOVE_DOWN_LEFT,
		MOVE_DOWN_RIGHT,
		WAIT,
		PICKUP,
		INVENTORY,
		DROP_ITEM,
		MESSAGE_HISTORY,
		RETURN_TO_GAME,
		NEW_GAME,
		QUIT,
		OPEN_PAUSE_MENU,
		DESCEND_STAIRS,
		SHOW_START_MENU
	};

	class Engine;

	class EventHandler
	{
	public:
		virtual ~EventHandler() = default;

		virtual void SetKeyMap(
		    const std::unordered_map<KeyPress, Actions, KeyPressHash>&
		        keyMap) = 0;
		virtual std::unique_ptr<Command> Dispatch() const = 0;
	};

	class BaseEventHandler : public EventHandler
	{
	public:
		BaseEventHandler(Engine& engine);

		void SetKeyMap(
		    const std::unordered_map<KeyPress, Actions, KeyPressHash>&
		        keyMap) override;

		std::unique_ptr<Command> Dispatch() const override;

	protected:
		std::unordered_map<KeyPress, Actions, KeyPressHash> keyMap_;
		Engine& engine_;
	};

	class MainGameEventHandler final : public BaseEventHandler
	{
	public:
		MainGameEventHandler(Engine& engine);
	};

	class MessageHistoryEventHandler final : public BaseEventHandler
	{
	public:
		MessageHistoryEventHandler(Engine& engine);
	};

	class GameOverEventHandler final : public BaseEventHandler
	{
	public:
		GameOverEventHandler(Engine& engine);
	};

	// Character creation event handler - handles input for character creation screen
	// This is kept separate from the menu stack system due to character creation's
	// unique tabbed interface
	class CharacterCreationEventHandler final : public BaseEventHandler
	{
	public:
		CharacterCreationEventHandler(Engine& engine);
		std::unique_ptr<Command> Dispatch() const override;
	};

	class InventoryEventHandler final : public BaseEventHandler
	{
	public:
		InventoryEventHandler(Engine& engine, bool isDropMode);

		std::unique_ptr<Command> Dispatch() const override;

	private:
		bool isDropMode_;
	};

	class ItemSelectionEventHandler final : public BaseEventHandler
	{
	public:
		ItemSelectionEventHandler(Engine& engine);

		std::unique_ptr<Command> Dispatch() const override;
	};
} // namespace tutorial

#endif // EVENT_HANDLER_HPP
