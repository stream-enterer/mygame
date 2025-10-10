#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include "Event.hpp"
#include "KeyPress.hpp"

#include <memory>
#include <unordered_map>

namespace tutorial
{
    enum Actions
    {
        MOVE_UP = 0,
        MOVE_DOWN,
        MOVE_LEFT,
        MOVE_RIGHT,
        WAIT,
        PICKUP,
        INVENTORY,
        MESSAGE_HISTORY,
        RETURN_TO_GAME,
        NEW_GAME,
        QUIT
    };

    class Engine;

    class EventHandler
    {
    public:
        virtual ~EventHandler() = default;

        virtual void SetKeyMap(
            const std::unordered_map<KeyPress, Actions, KeyPressHash>&
                keyMap) = 0;
        virtual std::unique_ptr<Event> Dispatch() const = 0;
    };

    class BaseEventHandler : public EventHandler
    {
    public:
        BaseEventHandler(Engine& engine);

        void SetKeyMap(const std::unordered_map<KeyPress, Actions,
                                                KeyPressHash>& keyMap) override;

        std::unique_ptr<Event> Dispatch() const override;

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

    class InventoryEventHandler final : public BaseEventHandler
    {
    public:
        InventoryEventHandler(Engine& engine);

        std::unique_ptr<Event> Dispatch() const override;
    };
} // namespace tutorial

#endif // EVENT_HANDLER_HPP
