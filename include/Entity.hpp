#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "AiComponent.hpp"
#include "Components.hpp"
#include "Item.hpp"
#include "Position.hpp"

#include <memory>
#include <string>

namespace tutorial
{
    class Engine;

    class Entity
    {
    public:
        using uint = unsigned int;

        virtual ~Entity() = default;

        virtual void Act(Engine& engine) = 0;
        virtual void Die() = 0;
        virtual void Use() = 0;
        virtual void SetPos(pos_t pos) = 0;

        virtual bool CanAct() const = 0;
        virtual AttackerComponent* GetAttacker() const = 0;
        virtual DestructibleComponent* GetDestructible() const = 0;
        virtual const std::string& GetName() const = 0;
        virtual const RenderableComponent* GetRenderable() const = 0;
        virtual pos_t GetPos() const = 0;
        virtual bool IsBlocker() const = 0;
    };
} // namespace tutorial

namespace tutorial
{
    class BaseEntity : public Entity
    {
    public:
        BaseEntity(pos_t pos, const std::string& name, bool blocker,
                   AttackerComponent attack,
                   const DestructibleComponent& defense,
                   const IconRenderable& renderable);

        virtual void Act(Engine& engine) override;
        virtual void Die() override;
        virtual void Use() override;
        virtual void SetPos(pos_t pos) override;

        virtual bool CanAct() const override;
        virtual AttackerComponent* GetAttacker() const override;
        virtual DestructibleComponent* GetDestructible() const override;
        virtual const std::string& GetName() const override;
        virtual const RenderableComponent* GetRenderable() const override;
        virtual pos_t GetPos() const override;
        virtual bool IsBlocker() const override;

    protected:
        std::string name_;
        std::unique_ptr<IconRenderable> renderable_;
        std::unique_ptr<DestructibleComponent> defense_;
        std::unique_ptr<AttackerComponent> attack_;
        pos_t pos_;
        bool blocker_;
    };
} // namespace tutorial

namespace tutorial
{
    class Npc : public BaseEntity
    {
    public:
        Npc(pos_t pos, const std::string& name, bool blocker,
            AttackerComponent attack, const DestructibleComponent& defense,
            const IconRenderable& renderable, std::unique_ptr<AiComponent> ai);

        void Act(Engine& engine) override;

    private:
        std::unique_ptr<AiComponent> ai_;
    };
} // namespace tutorial

namespace tutorial
{
    class Player : public BaseEntity
    {
    public:
        Player(pos_t pos, const std::string& name, bool blocker,
               AttackerComponent attack, const DestructibleComponent& defense,
               const IconRenderable& renderable);

        void Use() override;

    private:
        std::vector<Item> items_;
    };
} // namespace tutorial

#endif // ENTITY_HPP
