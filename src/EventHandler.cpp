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
        constexpr std::size_t kNumActions = 9;

        static const std::array<std::function<std::unique_ptr<Event>(Engine&)>,
                                kNumActions>
            kGameActions{
                // Up
                [](auto& engine)
                {
                    return std::make_unique<BumpAction>(
                        engine, *engine.GetPlayer(), pos_t{ 0, -1 });
                },
                // Down
                [](auto& engine)
                {
                    return std::make_unique<BumpAction>(
                        engine, *engine.GetPlayer(), pos_t{ 0, 1 });
                },
                // Left
                [](auto& engine)
                {
                    return std::make_unique<BumpAction>(
                        engine, *engine.GetPlayer(), pos_t{ -1, 0 });
                },
                // Right
                [](auto& engine)
                {
                    return std::make_unique<BumpAction>(
                        engine, *engine.GetPlayer(), pos_t{ 1, 0 });
                },
                // Wait action
                [](auto& engine)
                {
                    return std::make_unique<WaitAction>(engine,
                                                        *engine.GetPlayer());
                },
                // Message history event
                [](auto& engine)
                { return std::make_unique<MessageHistoryEvent>(engine); },
                // Return to game event
                [](auto& engine)
                { return std::make_unique<ReturnToGameEvent>(engine); },
                // New game
                [](auto& engine)
                { return std::make_unique<NewGameEvent>(engine); },
                // Exit
                [](auto& engine) { return std::make_unique<QuitEvent>(engine); }
            };
    } // namespace

    BaseEventHandler::BaseEventHandler(Engine& engine) : engine_(engine)
    {
    }

    void BaseEventHandler::SetKeyMap(
        const std::unordered_map<KeyPress, Actions, KeyPressHash>& keyMap)
    {
        keyMap_ = keyMap;
    }

    std::unique_ptr<Event> BaseEventHandler::Dispatch() const
    {
        // Use SDL3's event system instead of deprecated TCOD events
        SDL_Event sdlEvent;

        std::unique_ptr<Event> event{ nullptr };

        // Poll all pending events
        while (SDL_PollEvent(&sdlEvent))
        {
            // Check if it's a quit event (user closed window)
            if (sdlEvent.type == SDL_EVENT_QUIT)
            {
                return std::make_unique<QuitEvent>(engine_);
            }

            // Handle mouse motion
            if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION)
            {
                // Get window size in pixels
                SDL_Window* window =
                    TCOD_context_get_sdl_window(engine_.GetContext());
                int windowWidth, windowHeight;
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);

                // Convert pixel coordinates to tile coordinates
                int tileX = (sdlEvent.motion.x * engine_.GetConfig().width)
                            / windowWidth;
                int tileY = (sdlEvent.motion.y * engine_.GetConfig().height)
                            / windowHeight;

                // Store in engine
                engine_.SetMousePos(pos_t{ tileX, tileY });
            }

            // Check if it's a key press
            if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
            {
                // Convert SDL keycode to our KeyPress format
                SDL_Keycode sdlKey = sdlEvent.key.key;

                // Map SDL keycodes to TCOD keycodes for compatibility
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
                        // For letter keys, check if it's a printable character
                        if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z)
                        {
                            tcodKey = TCODK_CHAR;
                            character = static_cast<char>(sdlKey);
                        }
                        break;
                }

                try
                {
                    KeyPress keypress{ tcodKey, character };
                    auto action = keyMap_.at(keypress);
                    event = kGameActions.at(action)(engine_);

                    // Return the first valid event we find
                    if (event != nullptr)
                    {
                        return event;
                    }
                }
                catch (std::out_of_range&)
                {
                    // Key not mapped, continue checking other events
                }
            }
        }

        return event;
    }

    static const std::unordered_map<KeyPress, Actions, KeyPressHash>
        MainGameKeyMap{ { TCODK_UP, Actions::MOVE_UP },
                        { TCODK_DOWN, Actions::MOVE_DOWN },
                        { TCODK_LEFT, Actions::MOVE_LEFT },
                        { TCODK_RIGHT, Actions::MOVE_RIGHT },
                        { TCODK_SPACE, Actions::WAIT },
                        { { TCODK_CHAR, 'v' }, Actions::MESSAGE_HISTORY },
                        { TCODK_ENTER, Actions::NEW_GAME },
                        { TCODK_ESCAPE, Actions::QUIT } };

    MainGameEventHandler::MainGameEventHandler(Engine& engine) :
        BaseEventHandler(engine)
    {
        SetKeyMap(MainGameKeyMap);
    }

    static const std::unordered_map<KeyPress, Actions, KeyPressHash>
        MessageHistoryKeyMap{ { { TCODK_CHAR, 'v' },
                                Actions::RETURN_TO_GAME } };

    MessageHistoryEventHandler::MessageHistoryEventHandler(Engine& engine) :
        BaseEventHandler(engine)
    {
        SetKeyMap(MessageHistoryKeyMap);
    }

    static const std::unordered_map<KeyPress, Actions, KeyPressHash>
        GameOverKeyMap{ { TCODK_ENTER, Actions::NEW_GAME },
                        { TCODK_ESCAPE, Actions::QUIT } };

    GameOverEventHandler::GameOverEventHandler(Engine& engine) :
        BaseEventHandler(engine)
    {
        SetKeyMap(GameOverKeyMap);
    }
} // namespace tutorial
