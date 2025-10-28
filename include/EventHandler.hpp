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

	// Unified menu event handler - handles ALL menu input
	class MenuEventHandler final : public EventHandler
	{
	public:
		MenuEventHandler(Engine& engine);

		void SetKeyMap(
		    const std::unordered_map<KeyPress, Actions, KeyPressHash>&
		        /* keyMap */) override
		{
			// Menus don't use keymaps
		}

		std::unique_ptr<Command> Dispatch() const override;

	private:
		Engine& engine_;
	};

} // namespace tutorial

#endif // EVENT_HANDLER_HPP
