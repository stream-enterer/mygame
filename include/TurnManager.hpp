#ifndef TURN_MANAGER_HPP
#define TURN_MANAGER_HPP

#include <memory>

namespace tutorial
{
    class Command;
    class Engine;

    class TurnManager
    {
    public:
        // Process a player command and handle turn logic
        void ProcessCommand(std::unique_ptr<Command> command, Engine& engine);

    private:
        void ProcessEnemyTurn(Engine& engine);
    };

} // namespace tutorial

#endif // TURN_MANAGER_HPP
