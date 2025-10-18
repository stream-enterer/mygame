#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "AiComponent.hpp"
#include "Components.hpp"
#include "Item.hpp"
#include "Position.hpp"
#include "RenderLayer.hpp"

#include <memory>
#include <string>
#include <vector>
class Item; // Forward declaration

namespace tutorial
{

    enum class Faction
    {
        PLAYER,
        MONSTER,
        NEUTRAL
    };

    class Engine;
    class Entity
    {
    public:
        using uint = unsigned int;

        virtual ~Entity() = default;

        virtual void Act(Engine& engine) = 0;
        virtual void Die() = 0;
        virtual void Use(Engine& engine) = 0;
        virtual void SetPos(pos_t pos) = 0;
        virtual Item* GetItem() const = 0;
        virtual bool IsPickable() const = 0;
        virtual bool IsCorpse() const = 0;
        virtual bool CanAct() const = 0;
        virtual AttackerComponent* GetAttacker() const = 0;
        virtual DestructibleComponent* GetDestructible() const = 0;
        virtual const std::string& GetName() const = 0;
        virtual const RenderableComponent* GetRenderable() const = 0;
        virtual pos_t GetPos() const = 0;
        virtual bool IsBlocker() const = 0;
        virtual float GetDistance(int cx, int cy) const = 0;
        virtual Faction GetFaction() const = 0;
        virtual RenderLayer GetRenderLayer() const = 0;
        virtual int GetRenderPriority() const = 0;
        virtual void SetRenderPriority(int priority) = 0;
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
                   const IconRenderable& renderable, Faction faction,
                   std::unique_ptr<Item> item = nullptr, bool pickable = true,
                   bool isCorpse = false);

        virtual void Act(Engine& engine) override;
        virtual void Die() override;
        virtual void Use(Engine& engine) override;
        virtual void SetPos(pos_t pos) override;
        virtual Item* GetItem() const override;
        virtual bool IsPickable() const override;
        virtual bool IsCorpse() const override;
        virtual bool CanAct() const override;
        virtual AttackerComponent* GetAttacker() const override;
        virtual DestructibleComponent* GetDestructible() const override;
        virtual const std::string& GetName() const override;
        virtual const RenderableComponent* GetRenderable() const override;
        virtual pos_t GetPos() const override;
        virtual bool IsBlocker() const override;
        virtual float GetDistance(int cx, int cy) const override;
        virtual Faction GetFaction() const override;
        virtual RenderLayer GetRenderLayer() const override;
        virtual int GetRenderPriority() const override;
        virtual void SetRenderPriority(int priority) override;

    protected:
        std::string name_;
        std::unique_ptr<IconRenderable> renderable_;
        std::unique_ptr<DestructibleComponent> defense_;
        std::unique_ptr<AttackerComponent> attack_;
        std::unique_ptr<Item> item_;
        pos_t pos_;
        Faction faction_;
        bool blocker_;
        bool pickable_;
        bool isCorpse_;
        int renderPriority_;  // Higher = renders later (on top)
    };
} // namespace tutorial

namespace tutorial
{
    class Npc : public BaseEntity
    {
    public:
        Npc(pos_t pos, const std::string& name, bool blocker,
            AttackerComponent attack, const DestructibleComponent& defense,
            const IconRenderable& renderable, Faction faction,
            std::unique_ptr<AiComponent> ai, bool pickable = true,
            bool isCorpse = false);

        void Act(Engine& engine) override;
        std::unique_ptr<AiComponent> SwapAi(std::unique_ptr<AiComponent> newAi);

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
               const IconRenderable& renderable, Faction faction,
               bool pickable = true, bool isCorpse = false);

        void Use(Engine& engine) override;
        bool AddToInventory(std::unique_ptr<Entity> item);
        Entity* GetInventoryItem(size_t index);
        size_t GetInventorySize() const;
        const std::vector<std::unique_ptr<Entity>>& GetInventory() const;
        void RemoveFromInventory(size_t index);
        std::unique_ptr<Entity> ExtractFromInventory(size_t index);

    private:
        std::vector<std::unique_ptr<Entity>> inventory_;
    };
} // namespace tutorial

#endif // ENTITY_HPP
