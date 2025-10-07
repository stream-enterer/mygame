#ifndef AI_COMPONENT_HPP
#define AI_COMPONENT_HPP

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
} // namespace tutorial

#endif // AI_COMPONENT_HPP
