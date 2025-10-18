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
        constexpr std::size_t kNumActions = 12;

        static const std::array<
            std::function<std::unique_ptr<tutorial::Command>(Engine&)>,
            kNumActions>
            kGameActions{
                // Up
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::MoveCommand>(0, -1);
                },
                // Down
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::MoveCommand>(0, 1);
                },
                // Left
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::MoveCommand>(-1, 0);
                },
                // Right
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::MoveCommand>(1, 0);
                },
                // Wait action
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::WaitCommand>();
                },
                // Pickup action
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::PickupCommand>();
                },
                // Inventory event
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::OpenInventoryCommand>();
                },
                // Drop item event
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<
                        tutorial::OpenDropInventoryCommand>();
                },
                // Message history event
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<
                        tutorial::OpenMessageHistoryCommand>();
                },
                // Return to game event
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::CloseUICommand>();
                },
                // New game
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::NewGameCommand>();
                },
                // Exit
                [](auto& engine)
                {
                    (void)engine;
                    return std::make_unique<tutorial::QuitCommand>();
                }
            };
    }; // namespace

    tutorial::BaseEventHandler::BaseEventHandler(Engine& engine) :
        engine_(engine)
    {
    }

    void tutorial::BaseEventHandler::SetKeyMap(
        const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
                                 tutorial::KeyPressHash>& keyMap)
    {
        keyMap_ = keyMap;
    }

    std::unique_ptr<tutorial::Command> tutorial::BaseEventHandler::Dispatch()
        const
    {
        SDL_Event sdlEvent;
        std::unique_ptr<tutorial::Command> command{ nullptr };

        while (SDL_PollEvent(&sdlEvent))
        {
            if (sdlEvent.type == SDL_EVENT_QUIT)
            {
                return std::make_unique<tutorial::QuitCommand>();
            }

            if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION)
            {
                SDL_Window* window =
                    TCOD_context_get_sdl_window(engine_.GetContext());
                int windowWidth, windowHeight;
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);

                int tileX = (sdlEvent.motion.x * engine_.GetConfig().width)
                            / windowWidth;
                int tileY = (sdlEvent.motion.y * engine_.GetConfig().height)
                            / windowHeight;

                engine_.SetMousePos(tutorial::pos_t{ tileX, tileY });
            }

            if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Keycode sdlKey = sdlEvent.key.key;

                TCOD_keycode_t tcodKey = TCODK_NONE;
                char character = '\0';

                switch (sdlKey)
                {
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
                    case SDLK_SPACE:
                        tcodKey = TCODK_SPACE;
                        break;
                    case SDLK_RETURN:
                        tcodKey = TCODK_ENTER;
                        break;
                    case SDLK_ESCAPE:
                        tcodKey = TCODK_ESCAPE;
                        break;
                    default:
                        if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z)
                        {
                            tcodKey = TCODK_CHAR;
                            character = static_cast<char>(sdlKey);
                        }
                        break;
                }

                try
                {
                    tutorial::KeyPress keypress{ tcodKey, character };
                    auto action = keyMap_.at(keypress);
                    command = kGameActions.at(action)(engine_);

                    if (command != nullptr)
                    {
                        return command;
                    }
                }
                catch (std::out_of_range&)
                {
                    // Key not mapped, continue checking other events
                }
            }
        }

        return command;
    }

    static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
                                    tutorial::KeyPressHash>
        MainGameKeyMap{ { TCODK_UP, tutorial::Actions::MOVE_UP },
                        { TCODK_DOWN, tutorial::Actions::MOVE_DOWN },
                        { TCODK_LEFT, tutorial::Actions::MOVE_LEFT },
                        { TCODK_RIGHT, tutorial::Actions::MOVE_RIGHT },
                        { TCODK_SPACE, tutorial::Actions::WAIT },
                        { { TCODK_CHAR, 'g' }, tutorial::Actions::PICKUP },
                        { { TCODK_CHAR, 'i' }, tutorial::Actions::INVENTORY },
                        { { TCODK_CHAR, 'v' },
                          tutorial::Actions::MESSAGE_HISTORY },
                        { { TCODK_CHAR, 'd' }, tutorial::Actions::DROP_ITEM },
                        { TCODK_ENTER, tutorial::Actions::NEW_GAME } };

    tutorial::MainGameEventHandler::MainGameEventHandler(Engine& engine) :
        BaseEventHandler(engine)
    {
        SetKeyMap(MainGameKeyMap);
    }

    static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
                                    tutorial::KeyPressHash>
        MessageHistoryKeyMap{
            { { TCODK_CHAR, 'v' }, tutorial::Actions::RETURN_TO_GAME },
            { TCODK_ESCAPE, tutorial::Actions::RETURN_TO_GAME }
        };

    tutorial::MessageHistoryEventHandler::MessageHistoryEventHandler(
        Engine& engine) :
        BaseEventHandler(engine)
    {
        SetKeyMap(MessageHistoryKeyMap);
    }

    static const std::unordered_map<tutorial::KeyPress, tutorial::Actions,
                                    tutorial::KeyPressHash>
        GameOverKeyMap{ { TCODK_ENTER, tutorial::Actions::NEW_GAME },
                        { TCODK_ESCAPE, tutorial::Actions::QUIT } };

    tutorial::GameOverEventHandler::GameOverEventHandler(Engine& engine) :
        BaseEventHandler(engine)
    {
        SetKeyMap(GameOverKeyMap);
    }

    tutorial::InventoryEventHandler::InventoryEventHandler(Engine& engine) :
        BaseEventHandler(engine), mode_(InventoryMode::Use)
    {
    }

    std::unique_ptr<tutorial::Command>
    tutorial::InventoryEventHandler::Dispatch() const
    {
        SDL_Event sdlEvent;
        std::unique_ptr<tutorial::Command> command{ nullptr };

        while (SDL_PollEvent(&sdlEvent))
        {
            if (sdlEvent.type == SDL_EVENT_QUIT)
            {
                return std::make_unique<QuitCommand>();
            }

            // Right-click to close without consuming a turn
            if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                && sdlEvent.button.button == SDL_BUTTON_RIGHT)
            {
                return std::make_unique<tutorial::CloseUICommand>();
            }

            if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Keycode sdlKey = sdlEvent.key.key;

                // Escape or 'i' or 'd' returns to game WITHOUT using a turn
                if (sdlKey == SDLK_ESCAPE || sdlKey == SDLK_I)
                {
                    return std::make_unique<tutorial::CloseUICommand>();
                }

                // Check for a-z keys for item selection
                if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z)
                {
                    size_t itemIndex = sdlKey - SDLK_A;
                    if (mode_ == tutorial::InventoryMode::Drop)
                    {
                        return std::make_unique<tutorial::DropItemCommand>(
                            itemIndex);
                    }
                    else
                    {
                        return std::make_unique<tutorial::UseItemCommand>(
                            itemIndex);
                    }
                }
            }
        }

        return command;
    }

    tutorial::ItemSelectionEventHandler::ItemSelectionEventHandler(
        Engine& engine) :
        BaseEventHandler(engine)
    {
    }

    std::unique_ptr<tutorial::Command>
    tutorial::ItemSelectionEventHandler::Dispatch() const
    {
        SDL_Event sdlEvent;
        std::unique_ptr<tutorial::Command> command{ nullptr };

        while (SDL_PollEvent(&sdlEvent))
        {
            if (sdlEvent.type == SDL_EVENT_QUIT)
            {
                return std::make_unique<QuitCommand>();
            }

            // Right-click to close without consuming a turn
            if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                && sdlEvent.button.button == SDL_BUTTON_RIGHT)
            {
                return std::make_unique<tutorial::CloseUICommand>();
            }

            if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Keycode sdlKey = sdlEvent.key.key;

                // Escape closes menu without consuming a turn
                if (sdlKey == SDLK_ESCAPE)
                {
                    return std::make_unique<tutorial::CloseUICommand>();
                }

                // Check for a-z keys for item selection
                if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z)
                {
                    size_t itemIndex = sdlKey - SDLK_A;
                    const auto& items = engine_.GetItemSelectionList();

                    // Only accept valid item indices
                    if (itemIndex < items.size())
                    {
                        return std::make_unique<tutorial::PickupItemCommand>(
                            items[itemIndex]);
                    }
                    // Invalid key - don't do anything, don't close menu
                }
            }
        }

        return command;
    }

} // namespace tutorial
