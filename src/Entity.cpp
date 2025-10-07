#include "Entity.hpp"

#include "AiComponent.hpp"
#include "Colors.hpp"
#include "Components.hpp"
#include "Engine.hpp"
#include "Util.hpp"

namespace tutorial
{
    inline namespace
    {
        static const IconRenderable kDeadIcon { color::dark_red, '%' };
    }

    BaseEntity::BaseEntity(pos_t pos, const std::string& name, bool blocker,
                           AttackerComponent attack,
                           const DestructibleComponent& defense,
                           const IconRenderable& renderable)
        : name_(name),
          renderable_(std::make_unique<IconRenderable>(renderable)),
          defense_(std::make_unique<DestructibleComponent>(defense)),
          attack_(std::make_unique<AttackerComponent>(attack)),
          pos_(pos),
          blocker_(blocker)
    {
    }

    void BaseEntity::Act(Engine&)
    {
        // No op
    }

    void BaseEntity::Die()
    {
        renderable_ = std::make_unique<IconRenderable>(kDeadIcon);
        defense_ = nullptr;
        attack_ = nullptr;
        blocker_ = false;
        name_ = "remains of " + name_;
    }

    void BaseEntity::Use()
    {
        // no op
    }

    void BaseEntity::SetPos(pos_t pos)
    {
        pos_ = pos;
    }

    bool BaseEntity::CanAct() const
    {
        return (defense_ && !defense_->IsDead());
    }

    AttackerComponent* BaseEntity::GetAttacker() const
    {
        return attack_.get();
    }

    DestructibleComponent* BaseEntity::GetDestructible() const
    {
        return defense_.get();
    }

    const std::string& BaseEntity::GetName() const
    {
        return name_;
    }

    const RenderableComponent* BaseEntity::GetRenderable() const
    {
        return renderable_.get();
    }

    pos_t BaseEntity::GetPos() const
    {
        return pos_;
    }

    bool BaseEntity::IsBlocker() const
    {
        return blocker_;
    }
} // namespace tutorial

namespace tutorial
{
    Npc::Npc(pos_t pos, const std::string& name, bool blocker,
             AttackerComponent attack, const DestructibleComponent& defense,
             const IconRenderable& renderable, std::unique_ptr<AiComponent> ai)
        : BaseEntity(pos, name, blocker, attack, defense, renderable),
          ai_(std::move(ai))
    {
    }

    void Npc::Act(Engine& engine)
    {
        if (defense_ && defense_->IsDead())
        {
            return;
        }

        ai_->Perform(engine, *this);
    }
} // namespace tutorial

namespace tutorial
{
    Player::Player(pos_t pos, const std::string& name, bool blocker,
                   AttackerComponent attack,
                   const DestructibleComponent& defense,
                   const IconRenderable& renderable)
        : BaseEntity(pos, name, blocker, attack, defense, renderable)
    {
    }

    void Player::Use()
    {
        auto item = items_.begin();

        if (item != items_.end())
        {
            item->Use(*this);
            items_.erase(item);
        }
    }
} // namespace tutorial
