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

	// Base class for menu-based event handlers with common navigation
	// logic Reduces ~120 lines of duplicated code across 4 menu handlers
	class MenuEventHandlerBase : public BaseEventHandler
	{
	public:
		MenuEventHandlerBase(Engine& engine);

		std::unique_ptr<Command> Dispatch() const override;

	protected:
		// Subclasses override to customize ESC key behavior
		// Return nullptr if ESC should be ignored
		virtual std::unique_ptr<Command> HandleEscape() const = 0;
	};

	class PauseMenuEventHandler final : public MenuEventHandlerBase
	{
	public:
		PauseMenuEventHandler(Engine& engine);

	protected:
		std::unique_ptr<Command> HandleEscape() const override;
	};

	class StartMenuEventHandler final : public MenuEventHandlerBase
	{
	public:
		StartMenuEventHandler(Engine& engine);

	protected:
		std::unique_ptr<Command> HandleEscape() const override;
	};

	class CharacterCreationEventHandler final : public MenuEventHandlerBase
	{
	public:
		CharacterCreationEventHandler(Engine& engine);

	protected:
		std::unique_ptr<Command> HandleEscape() const override;
	};

	class LevelUpMenuEventHandler final : public MenuEventHandlerBase
	{
	public:
		LevelUpMenuEventHandler(Engine& engine);

	protected:
		std::unique_ptr<Command> HandleEscape() const override;
	};

	class InventoryEventHandler final : public BaseEventHandler
	{
	public:
		InventoryEventHandler(Engine& engine);

		std::unique_ptr<Command> Dispatch() const override;
		void SetMode(InventoryMode mode)
		{
			mode_ = mode;
		}
		InventoryMode GetMode() const
		{
			return mode_;
		}

	private:
		InventoryMode mode_;
	};

	class ItemSelectionEventHandler final : public BaseEventHandler
	{
	public:
		ItemSelectionEventHandler(Engine& engine);

		std::unique_ptr<Command> Dispatch() const override;
	};
} // namespace tutorial

#endif // EVENT_HANDLER_HPP
