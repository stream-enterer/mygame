#include "Entity.hpp"

#include "AiComponent.hpp"
#include "Colors.hpp"
#include "Components.hpp"
#include "Engine.hpp"
#include "Util.hpp"

#include <cmath>

namespace tutorial
{
    inline namespace
    {
        static const IconRenderable kDeadIcon{ color::dark_red, '%' };
    }

    BaseEntity::BaseEntity(pos_t pos, const std::string& name, bool blocker,
                           AttackerComponent attack,
                           const DestructibleComponent& defense,
                           const IconRenderable& renderable, Faction faction,
                           std::unique_ptr<Item> item) :
        name_(name),
        renderable_(std::make_unique<IconRenderable>(renderable)),
        defense_(std::make_unique<DestructibleComponent>(defense)),
        attack_(std::make_unique<AttackerComponent>(attack)),
        item_(std::move(item)),
        pos_(pos),
        faction_(faction),
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

    void BaseEntity::Use(Engine& engine)
    {
        if (item_)
        {
            item_->Use(*this, engine);
        }
    }

    void BaseEntity::SetPos(pos_t pos)
    {
        pos_ = pos;
    }

    Item* BaseEntity::GetItem() const
    {
        return item_.get();
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

    float BaseEntity::GetDistance(int cx, int cy) const
    {
        int dx = pos_.x - cx;
        int dy = pos_.y - cy;
        return std::sqrt(dx * dx + dy * dy);
    }

    Faction BaseEntity::GetFaction() const
    {
        return faction_;
    }
} // namespace tutorial

namespace tutorial
{
    Npc::Npc(pos_t pos, const std::string& name, bool blocker,
             AttackerComponent attack, const DestructibleComponent& defense,
             const IconRenderable& renderable, Faction faction,
             std::unique_ptr<AiComponent> ai) :
        BaseEntity(pos, name, blocker, attack, defense, renderable, faction),
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

    std::unique_ptr<AiComponent> Npc::SwapAi(std::unique_ptr<AiComponent> newAi)
    {
        std::unique_ptr<AiComponent> oldAi = std::move(ai_);
        ai_ = std::move(newAi);
        return oldAi;
    }
} // namespace tutorial

namespace tutorial
{
    Player::Player(pos_t pos, const std::string& name, bool blocker,
                   AttackerComponent attack,
                   const DestructibleComponent& defense,
                   const IconRenderable& renderable, Faction faction) :
        BaseEntity(pos, name, blocker, attack, defense, renderable, faction)
    {
    }

    void Player::Use(Engine& engine)
    {
        // Using an item from inventory is now handled by UseItemAction
        // This method is for when the player entity itself is "used" (not
        // applicable)
    }

    bool Player::AddToInventory(std::unique_ptr<Entity> item)
    {
        if (inventory_.size() >= kMaxInventorySize)
        {
            return false;
        }
        inventory_.push_back(std::move(item));
        return true;
    }

    Entity* Player::GetInventoryItem(size_t index)
    {
        if (index < inventory_.size())
        {
            return inventory_[index].get();
        }
        return nullptr;
    }

    size_t Player::GetInventorySize() const
    {
        return inventory_.size();
    }

    const std::vector<std::unique_ptr<Entity>>& Player::GetInventory() const
    {
        return inventory_;
    }

    void Player::RemoveFromInventory(size_t index)
    {
        if (index < inventory_.size())
        {
            inventory_.erase(inventory_.begin() + index);
        }
    }

    std::unique_ptr<Entity> Player::ExtractFromInventory(size_t index)
    {
        if (index < inventory_.size())
        {
            auto item = std::move(inventory_[index]);
            inventory_.erase(inventory_.begin() + index);
            return item;
        }
        return nullptr;
    }

} // namespace tutorial
