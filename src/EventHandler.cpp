#include "EventHandler.hpp"

#include "Engine.hpp"
#include "Event.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace tutorial
{
	inline namespace
	{
		constexpr std::size_t kNumActions = 19;

		static const std::array<
		    std::function<std::unique_ptr<tutorial::Command>(Engine&)>,
		    kNumActions>
		    kGameActions {
			    // Up
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(0, -1);
			    },
			    // Down
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(0, 1);
			    },
			    // Left
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(-1, 0);
			    },
			    // Right
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(1, 0);
			    },
			    // Up-Left
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(-1, -1);
			    },
			    // Up-Right
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(1, -1);
			    },
			    // Down-Left
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(-1, 1);
			    },
			    // Down-Right
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::MoveCommand>(1, 1);
			    },
			    // Wait action
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::WaitCommand>();
			    },
			    // Pickup action
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::PickupCommand>();
			    },
			    // Inventory event
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::OpenInventoryCommand>();
			    },
			    // Drop item event
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::OpenDropInventoryCommand>();
			    },
			    // Message history event
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::OpenMessageHistoryCommand>();
			    },
			    // Return to game event
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::CloseUICommand>();
			    },
			    // New game
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::NewGameCommand>();
			    },
			    // Exit
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::QuitCommand>();
			    },
			    // Open pause menu
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::OpenPauseMenuCommand>();
			    },
			    // Descend stairs
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::DescendStairsCommand>();
			    },
			    // Show start menu
			    [](auto& engine) {
			            (void)engine;
			            return std::make_unique<
			                tutorial::StartMenuCommand>();
			    }
		    };
	}; // namespace

	tutorial::BaseEventHandler::BaseEventHandler(Engine& engine)
	    : engine_(engine)
	{
	}

	void tutorial::BaseEventHandler::SetKeyMap(
	    const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
	                             tutorial::KeyPressHash>& keyMap)
	{
		keyMap_ = keyMap;
	}

	std::unique_ptr<tutorial::Command>
	tutorial::BaseEventHandler::Dispatch() const
	{
		SDL_Event sdlEvent;
		std::unique_ptr<tutorial::Command> command { nullptr };

		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_EVENT_QUIT) {
				return std::make_unique<
				    tutorial::QuitCommand>();
			}

			if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION) {
				// Use libtcod's coordinate conversion which
				// handles viewport transforms
				int tileX = static_cast<int>(sdlEvent.motion.x);
				int tileY = static_cast<int>(sdlEvent.motion.y);
				TCOD_context_screen_pixel_to_tile_i(
				    engine_.GetContext(), &tileX, &tileY);

				engine_.SetMousePos(
				    tutorial::pos_t { tileX, tileY });
			}

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode sdlKey = sdlEvent.key.key;

				TCOD_keycode_t tcodKey = TCODK_NONE;
				char character = '\0';

				switch (sdlKey) {
					case SDLK_UP:
						tcodKey = TCODK_UP;
						break;
					case SDLK_DOWN:
						tcodKey = TCODK_DOWN;
						break;
					case SDLK_LEFT:
						tcodKey = TCODK_LEFT;
						break;
					case SDLK_RIGHT:
						tcodKey = TCODK_RIGHT;
						break;
					case SDLK_KP_8:
						tcodKey = TCODK_UP;
						break;
					case SDLK_KP_2:
						tcodKey = TCODK_DOWN;
						break;
					case SDLK_KP_4:
						tcodKey = TCODK_LEFT;
						break;
					case SDLK_KP_6:
						tcodKey = TCODK_RIGHT;
						break;
					case SDLK_KP_7:
						tcodKey = TCODK_KP7;
						break;
					case SDLK_KP_9:
						tcodKey = TCODK_KP9;
						break;
					case SDLK_KP_1:
						tcodKey = TCODK_KP1;
						break;
					case SDLK_KP_3:
						tcodKey = TCODK_KP3;
						break;
					case SDLK_KP_5:
						tcodKey = TCODK_KP5;
						break;
					case SDLK_SPACE:
						tcodKey = TCODK_SPACE;
						break;
					case SDLK_RETURN:
						tcodKey = TCODK_ENTER;
						break;
					case SDLK_ESCAPE:
						tcodKey = TCODK_ESCAPE;
						break;
					case SDLK_PERIOD:
						// Handle '>' (Shift+Period) for
						// stairs
						if (sdlEvent.key.mod
						    & SDL_KMOD_SHIFT) {
							tcodKey = TCODK_CHAR;
							character = '>';
						} else {
							tcodKey = TCODK_CHAR;
							character = '.';
						}
						break;
					default:
						if (sdlKey >= SDLK_A
						    && sdlKey <= SDLK_Z) {
							tcodKey = TCODK_CHAR;
							character =
							    static_cast<char>(
							        sdlKey);
						}
						break;
				}

				try {
					tutorial::KeyPress keypress {
						tcodKey, character
					};
					auto action = keyMap_.at(keypress);
					command =
					    kGameActions.at(action)(engine_);

					if (command != nullptr) {
						return command;
					}
				} catch (std::out_of_range&) {
					// Key not mapped, continue checking
					// other events
				}
			}
		}

		return command;
	}

	static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
	                                tutorial::KeyPressHash>
	    MainGameKeyMap {
		    { TCODK_UP, tutorial::Actions::MOVE_UP },
		    { TCODK_DOWN, tutorial::Actions::MOVE_DOWN },
		    { TCODK_LEFT, tutorial::Actions::MOVE_LEFT },
		    { TCODK_RIGHT, tutorial::Actions::MOVE_RIGHT },
		    { TCODK_KP7, tutorial::Actions::MOVE_UP_LEFT },
		    { TCODK_KP9, tutorial::Actions::MOVE_UP_RIGHT },
		    { TCODK_KP1, tutorial::Actions::MOVE_DOWN_LEFT },
		    { TCODK_KP3, tutorial::Actions::MOVE_DOWN_RIGHT },
		    { TCODK_SPACE, tutorial::Actions::WAIT },
		    { TCODK_KP5, tutorial::Actions::WAIT },
		    { { TCODK_CHAR, 'g' }, tutorial::Actions::PICKUP },
		    { { TCODK_CHAR, 'i' }, tutorial::Actions::INVENTORY },
		    { { TCODK_CHAR, 'v' }, tutorial::Actions::MESSAGE_HISTORY },
		    { { TCODK_CHAR, 'd' }, tutorial::Actions::DROP_ITEM },
		    { { TCODK_CHAR, '>' }, tutorial::Actions::DESCEND_STAIRS },
		    { TCODK_ESCAPE, tutorial::Actions::OPEN_PAUSE_MENU }
	    };

	tutorial::MainGameEventHandler::MainGameEventHandler(Engine& engine)
	    : BaseEventHandler(engine)
	{
		SetKeyMap(MainGameKeyMap);
	}

	static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
	                                tutorial::KeyPressHash>
	    MessageHistoryKeyMap {
		    { { TCODK_CHAR, 'v' }, tutorial::Actions::RETURN_TO_GAME },
		    { TCODK_ESCAPE, tutorial::Actions::RETURN_TO_GAME }
	    };

	tutorial::MessageHistoryEventHandler::MessageHistoryEventHandler(
	    Engine& engine)
	    : BaseEventHandler(engine)
	{
		SetKeyMap(MessageHistoryKeyMap);
	}

	static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
	                                tutorial::KeyPressHash>
	    GameOverKeyMap { { TCODK_ESCAPE,
		               tutorial::Actions::SHOW_START_MENU } };

	tutorial::GameOverEventHandler::GameOverEventHandler(Engine& engine)
	    : BaseEventHandler(engine)
	{
		SetKeyMap(GameOverKeyMap);
	}

	// MenuEventHandlerBase - Common implementation for all menu handlers
	MenuEventHandlerBase::MenuEventHandlerBase(Engine& engine)
	    : BaseEventHandler(engine)
	{
	}

	std::unique_ptr<Command> MenuEventHandlerBase::Dispatch() const
	{
		SDL_Event sdlEvent;

		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_EVENT_QUIT) {
				return std::make_unique<QuitCommand>();
			}

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode sdlKey = sdlEvent.key.key;

				// UP key - select previous item
				if (sdlKey == SDLK_UP) {
					return std::make_unique<
					    MenuNavigateUpCommand>();
				}

				// DOWN key - select next item
				if (sdlKey == SDLK_DOWN) {
					return std::make_unique<
					    MenuNavigateDownCommand>();
				}

				// ENTER or SPACE - confirm selection
				if (sdlKey == SDLK_RETURN
				    || sdlKey == SDLK_SPACE) {
					return std::make_unique<
					    MenuConfirmCommand>();
				}

				// ESCAPE - delegate to subclass
				if (sdlKey == SDLK_ESCAPE) {
					auto escapeCmd = HandleEscape();
					if (escapeCmd) {
						return escapeCmd;
					}
					// If HandleEscape returns nullptr,
					// ignore ESC
					continue;
				}
			}
		}

		return nullptr;
	}

	// PauseMenuEventHandler - ESC closes menu and returns to game
	PauseMenuEventHandler::PauseMenuEventHandler(Engine& engine)
	    : MenuEventHandlerBase(engine)
	{
	}

	std::unique_ptr<Command> PauseMenuEventHandler::HandleEscape() const
	{
		return std::make_unique<CloseUICommand>();
	}

	// StartMenuEventHandler - ESC does nothing (must explicitly select
	// Exit)
	StartMenuEventHandler::StartMenuEventHandler(Engine& engine)
	    : MenuEventHandlerBase(engine)
	{
	}

	std::unique_ptr<Command> StartMenuEventHandler::HandleEscape() const
	{
		return nullptr; // Ignore ESC
	}

	// CharacterCreationEventHandler - ESC returns to start menu
	CharacterCreationEventHandler::CharacterCreationEventHandler(
	    Engine& engine)
	    : MenuEventHandlerBase(engine)
	{
	}

	std::unique_ptr<Command>
	CharacterCreationEventHandler::HandleEscape() const
	{
		return std::make_unique<StartMenuCommand>();
	}

	// LevelUpMenuEventHandler - ESC disabled (must choose stat upgrade)
	LevelUpMenuEventHandler::LevelUpMenuEventHandler(Engine& engine)
	    : MenuEventHandlerBase(engine)
	{
	}

	std::unique_ptr<Command> LevelUpMenuEventHandler::HandleEscape() const
	{
		return nullptr; // Ignore ESC - player must choose
	}

	tutorial::InventoryEventHandler::InventoryEventHandler(Engine& engine)
	    : BaseEventHandler(engine), mode_(InventoryMode::Use)
	{
	}

	std::unique_ptr<tutorial::Command>
	tutorial::InventoryEventHandler::Dispatch() const
	{
		SDL_Event sdlEvent;
		std::unique_ptr<tutorial::Command> command { nullptr };

		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_EVENT_QUIT) {
				return std::make_unique<QuitCommand>();
			}

			// Right-click to close without consuming a turn
			if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
			    && sdlEvent.button.button == SDL_BUTTON_RIGHT) {
				return std::make_unique<
				    tutorial::CloseUICommand>();
			}

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode sdlKey = sdlEvent.key.key;

				// Escape or 'i' or 'd' returns to game WITHOUT
				// using a turn
				if (sdlKey == SDLK_ESCAPE || sdlKey == SDLK_I) {
					return std::make_unique<
					    tutorial::CloseUICommand>();
				}

				// Check for a-z keys for item selection
				if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z) {
					size_t itemIndex = sdlKey - SDLK_A;
					if (mode_
					    == tutorial::InventoryMode::Drop) {
						return std::make_unique<
						    tutorial::DropItemCommand>(
						    itemIndex);
					} else {
						return std::make_unique<
						    tutorial::UseItemCommand>(
						    itemIndex);
					}
				}
			}
		}

		return command;
	}

	tutorial::ItemSelectionEventHandler::ItemSelectionEventHandler(
	    Engine& engine)
	    : BaseEventHandler(engine)
	{
	}

	std::unique_ptr<tutorial::Command>
	tutorial::ItemSelectionEventHandler::Dispatch() const
	{
		SDL_Event sdlEvent;
		std::unique_ptr<tutorial::Command> command { nullptr };

		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_EVENT_QUIT) {
				return std::make_unique<QuitCommand>();
			}

			// Right-click to close without consuming a turn
			if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
			    && sdlEvent.button.button == SDL_BUTTON_RIGHT) {
				return std::make_unique<
				    tutorial::CloseUICommand>();
			}

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode sdlKey = sdlEvent.key.key;

				// Escape closes menu without consuming a turn
				if (sdlKey == SDLK_ESCAPE) {
					return std::make_unique<
					    tutorial::CloseUICommand>();
				}

				// Check for a-z keys for item selection
				if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z) {
					size_t itemIndex = sdlKey - SDLK_A;
					const auto& items =
					    engine_.GetItemSelectionList();

					// Only accept valid item indices
					if (itemIndex < items.size()) {
						return std::make_unique<
						    tutorial::
						        PickupItemCommand>(
						    items[itemIndex]);
					}
					// Invalid key - don't do anything,
					// don't close menu
				}
			}
		}

		return command;
	}

} // namespace tutorial
