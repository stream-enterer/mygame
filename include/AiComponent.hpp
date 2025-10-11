#ifndef AI_COMPONENT_HPP
#define AI_COMPONENT_HPP

#include <memory>

namespace tutorial
{
    class Engine;
    class Entity;

    class AiComponent
    {
    public:
        virtual ~AiComponent() = default;

        virtual void Perform(Engine& engine, Entity& entity) = 0;
    };

    class BaseAi : public AiComponent
    {
    public:
        virtual void Perform(Engine& engine, Entity& entity) override;
    };

    class HostileAi : public BaseAi
    {
    public:
        void Perform(Engine& engine, Entity& entity) override;
    };

    class ConfusedMonsterAi : public AiComponent // ADD THIS CLASS
    {
    public:
        ConfusedMonsterAi(int nbTurns, std::unique_ptr<AiComponent> oldAi);

        void Perform(Engine& engine, Entity& entity) override;

    private:
        int nbTurns_;
        std::unique_ptr<AiComponent> oldAi_;
    };
} // namespace tutorial

#endif // AI_COMPONENT_HPP
