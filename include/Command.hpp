#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <memory>

namespace tutorial
{
    class Engine;
    class Entity;

    // Base command interface
    class Command
    {
    public:
        virtual ~Command() = default;

        // Execute the command
        virtual void Execute(Engine& engine) = 0;

        // Does this command consume a turn?
        virtual bool ConsumesTurn() = 0;
    };

    // UI commands that don't consume turns
    class OpenInventoryCommand final : public Command
    {
    public:
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return false;
        }
    };

    class OpenDropInventoryCommand final : public Command
    {
    public:
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return false;
        }
    };

    class OpenMessageHistoryCommand final : public Command
    {
    public:
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return false;
        }
    };

    class CloseUICommand final : public Command
    {
    public:
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return false;
        }
    };

    class NewGameCommand final : public Command
    {
    public:
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return false;
        }
    };

    class QuitCommand final : public Command
    {
    public:
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return false;
        }
    };

    // Gameplay commands that consume turns
    class ActionCommand : public Command
    {
    public:
        bool ConsumesTurn() override
        {
            return true;
        }
    };

    class MoveCommand final : public Command
    {
    public:
        MoveCommand(int dx, int dy) : dx_(dx), dy_(dy)
        {
        }
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override;

    private:
        int dx_;
        int dy_;
        mutable bool consumesTurn_ = true;
    };

    class WaitCommand final : public ActionCommand
    {
    public:
        void Execute(Engine& engine) override;
    };

    class PickupCommand final : public ActionCommand
    {
    public:
        void Execute(Engine& engine) override;
    };

    class PickupItemCommand final : public ActionCommand
    {
    public:
        PickupItemCommand(Entity* item) : item_(item)
        {
        }
        void Execute(Engine& engine) override;

    private:
        Entity* item_;
    };

    class UseItemCommand final : public Command
    {
    public:
        explicit UseItemCommand(size_t itemIndex) :
            itemIndex_(itemIndex), consumedTurn_(true)
        {
        }
        void Execute(Engine& engine) override;
        bool ConsumesTurn() override
        {
            return consumedTurn_;
        }

    private:
        size_t itemIndex_;
        bool consumedTurn_;
    };

    class DropItemCommand final : public ActionCommand
    {
    public:
        DropItemCommand(size_t itemIndex) : itemIndex_(itemIndex)
        {
        }
        void Execute(Engine& engine) override;

    private:
        size_t itemIndex_;
    };

} // namespace tutorial

#endif // COMMAND_HPP
