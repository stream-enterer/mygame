#include "EventHandler.hpp"

#include "Engine.hpp"
#include "Menu.hpp"

#include <iostream>
#include <unordered_map>

namespace tutorial
{
	namespace
	{
		static const std::unordered_map<
		    Actions, std::unique_ptr<Command> (*)(Engine&)> {
			tutorial::Actions::MOVE_UP,
			[](auto& engine) {
			        (void)engine;
			        return std::make_unique<tutorial::MoveCommand>(
			            0, -1);
			}
		},
		    { tutorial::Actions::MOVE_DOWN,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(0,
			                                                     1);
		      } },
		    { tutorial::Actions::MOVE_LEFT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(-1,
			                                                     0);
		      } },
		    { tutorial::Actions::MOVE_RIGHT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(1,
			                                                     0);
		      } },
		    { tutorial::Actions::MOVE_UP_LEFT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(
			          -1, -1);
		      } },
		    { tutorial::Actions::MOVE_UP_RIGHT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(
			          1, -1);
		      } },
		    { tutorial::Actions::MOVE_DOWN_LEFT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(-1,
			                                                     1);
		      } },
		    { tutorial::Actions::MOVE_DOWN_RIGHT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::MoveCommand>(1,
			                                                     1);
		      } },
		    { tutorial::Actions::WAIT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::WaitCommand>();
		      } },
		    { tutorial::Actions::PICKUP,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<
			          tutorial::PickupCommand>();
		      } },
		    { tutorial::Actions::INVENTORY,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<
			          tutorial::OpenInventoryCommand>();
		      } },
		    { tutorial::Actions::DROP_ITEM,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<
			          tutorial::OpenDropInventoryCommand>();
		      } },
		    { tutorial::Actions::MESSAGE_HISTORY,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<
			          tutorial::OpenMessageHistoryCommand>();
		      } },
		    { tutorial::Actions::RETURN_TO_GAME,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<
			          tutorial::CloseUICommand>();
		      } },
		    { tutorial::Actions::QUIT,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<tutorial::QuitCommand>();
		      } },
		    { tutorial::Actions::OPEN_PAUSE_MENU,
		      [](auto& engine) {
			      (void)engine;
			      return std::make_unique<
			          tutorial::OpenPauseMenuCommand>();
		      } },
		    { tutorial::Actions::DESCEND_STAIRS, [](auto& engine) {
			     (void)engine;
			     return std::make_unique<
			         tutorial::DescendStairsCommand>();
		     } },
		{
			tutorial::Actions::SHOW_START_MENU, [](auto& engine) {
				(void)engine;
				return std::make_unique<
				    tutorial::StartMenuCommand>();
			}
		}
	};
}; // namespace

BaseEventHandler::BaseEventHandler(Engine& engine) : engine_(engine)
{
}

void BaseEventHandler::SetKeyMap(
    const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
                             tutorial::KeyPressHash>& keyMap)
{
	keyMap_ = keyMap;
}

std::unique_ptr<tutorial::Command> BaseEventHandler::Dispatch() const
{
	SDL_Event sdlEvent;
	std::unique_ptr<tutorial::Command> command { nullptr };

	while (SDL_PollEvent(&sdlEvent)) {
		if (sdlEvent.type == SDL_EVENT_QUIT) {
			return std::make_unique<tutorial::QuitCommand>();
		}

		if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION) {
			// Use libtcod's coordinate conversion
			int tileX = static_cast<int>(sdlEvent.motion.x);
			int tileY = static_cast<int>(sdlEvent.motion.y);
			TCOD_context_screen_pixel_to_tile_i(
			    engine_.GetContext(), &tileX, &tileY);
			engine_.SetMousePos(pos_t { tileX, tileY });
		}

		if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
			SDL_Keycode sdlKey = sdlEvent.key.key;

			// Convert SDL keycode to TCOD keycode
			TCOD_keycode_t tcodKey = TCODK_NONE;
			char character = 0;

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
				case SDLK_ESCAPE:
					tcodKey = TCODK_ESCAPE;
					break;
				case SDLK_SPACE:
					tcodKey = TCODK_SPACE;
					break;
				case SDLK_KP_1:
					tcodKey = TCODK_KP1;
					break;
				case SDLK_KP_2:
					tcodKey = TCODK_KP2;
					break;
				case SDLK_KP_3:
					tcodKey = TCODK_KP3;
					break;
				case SDLK_KP_4:
					tcodKey = TCODK_KP4;
					break;
				case SDLK_KP_5:
					tcodKey = TCODK_KP5;
					break;
				case SDLK_KP_6:
					tcodKey = TCODK_KP6;
					break;
				case SDLK_KP_7:
					tcodKey = TCODK_KP7;
					break;
				case SDLK_KP_8:
					tcodKey = TCODK_KP8;
					break;
				case SDLK_KP_9:
					tcodKey = TCODK_KP9;
					break;
				default:
					if (sdlKey >= SDLK_A
					    && sdlKey <= SDLK_Z) {
						tcodKey = TCODK_CHAR;
						character =
						    static_cast<char>(sdlKey);
					}
					break;
			}

			try {
				tutorial::KeyPress keypress { tcodKey,
					                      character };
				auto action = keyMap_.at(keypress);
				command = kGameActions.at(action)(engine_);

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
    MainGameKeyMap { { TCODK_UP, tutorial::Actions::MOVE_UP },
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
	             { { TCODK_CHAR, 'v' },
	               tutorial::Actions::MESSAGE_HISTORY },
	             { { TCODK_CHAR, 'd' }, tutorial::Actions::DROP_ITEM },
	             { { TCODK_CHAR, '>' }, tutorial::Actions::DESCEND_STAIRS },
	             { TCODK_ESCAPE, tutorial::Actions::OPEN_PAUSE_MENU } };

MainGameEventHandler::MainGameEventHandler(Engine& engine)
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

MessageHistoryEventHandler::MessageHistoryEventHandler(Engine& engine)
    : BaseEventHandler(engine)
{
	SetKeyMap(MessageHistoryKeyMap);
}

static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
                                tutorial::KeyPressHash>
    GameOverKeyMap { { TCODK_ESCAPE, tutorial::Actions::SHOW_START_MENU } };

GameOverEventHandler::GameOverEventHandler(Engine& engine)
    : BaseEventHandler(engine)
{
	SetKeyMap(GameOverKeyMap);
}

// MenuEventHandler - Unified handler for all menus
MenuEventHandler::MenuEventHandler(Engine& engine) : engine_(engine)
{
}

std::unique_ptr<Command> MenuEventHandler::Dispatch() const
{
	SDL_Event sdlEvent;

	while (SDL_PollEvent(&sdlEvent)) {
		if (sdlEvent.type == SDL_EVENT_QUIT) {
			return std::make_unique<QuitCommand>();
		}

		if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
			SDL_Keycode sdlKey = sdlEvent.key.key;

			// Convert SDL key to character (for letter
			// selection)
			char character = 0;
			if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z) {
				character =
				    static_cast<char>(sdlKey - SDLK_A + 'a');
			}

			// Let Engine handle menu input
			const_cast<Engine&>(engine_).HandleMenuInput(sdlKey,
			                                             character);

			// Menu handles input directly, no command
			// needed
			return nullptr;
		}
	}

	return nullptr;
}

} // namespace tutorial
