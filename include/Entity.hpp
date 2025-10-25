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

	enum class Faction { PLAYER, MONSTER, NEUTRAL };

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
		virtual void SetName(const std::string& name) = 0;
		virtual const RenderableComponent* GetRenderable() const = 0;
		virtual pos_t GetPos() const = 0;
		virtual bool IsBlocker() const = 0;
		virtual float GetDistance(int cx, int cy) const = 0;
		virtual Faction GetFaction() const = 0;
		virtual RenderLayer GetRenderLayer() const = 0;
		virtual int GetRenderPriority() const = 0;
		virtual void SetRenderPriority(int priority) = 0;

		// Null-safety helpers - throw if component doesn't exist
		AttackerComponent& RequireAttacker() const
		{
			auto* component = GetAttacker();
			if (!component) {
				throw std::runtime_error(
				    "Entity " + GetName()
				    + " requires Attacker component");
			}
			return *component;
		}

		DestructibleComponent& RequireDestructible() const
		{
			auto* component = GetDestructible();
			if (!component) {
				throw std::runtime_error(
				    "Entity " + GetName()
				    + " requires Destructible component");
			}
			return *component;
		}

		Item& RequireItem() const
		{
			auto* component = GetItem();
			if (!component) {
				throw std::runtime_error(
				    "Entity " + GetName()
				    + " requires Item component");
			}
			return *component;
		}
	};
} // namespace tutorial

namespace tutorial
{
	/**
	 * BaseEntity - General-purpose entity with flexible component
	 * composition
	 *
	 * Required components:
	 * - Renderable (always present via IconRenderable)
	 * - Destructible (always present, may have 0 HP for non-living
	 * entities)
	 * - Attacker (always present, may have 0 power for non-combat entities)
	 *
	 * Optional components:
	 * - Item (present only for pickup-able items)
	 * - AI (not present in BaseEntity, see Npc class)
	 *
	 * Component invariants:
	 * - If blocker_ is true, the entity blocks movement
	 * - If pickable_ is true, the entity can be picked up (requires Item
	 * component)
	 * - If isCorpse_ is true, the entity renders on CORPSES layer
	 */
	class BaseEntity : public Entity
	{
	public:
		BaseEntity(pos_t pos, const std::string& name, bool blocker,
		           AttackerComponent attack,
		           const DestructibleComponent& defense,
		           const IconRenderable& renderable, Faction faction,
		           std::unique_ptr<Item> item = nullptr,
		           bool pickable = true, bool isCorpse = false);

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
		virtual void SetName(const std::string& name) override;
		virtual const RenderableComponent* GetRenderable()
		    const override;
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
		int renderPriority_; // Higher = renders later (on top)
	};
} // namespace tutorial

namespace tutorial
{
	/**
	 * Npc - Entity with AI behavior (monsters, NPCs)
	 *
	 * Required components (in addition to BaseEntity components):
	 * - AI (always present, defines behavior)
	 *
	 * Typical configuration:
	 * - blocker_ = true (NPCs block movement)
	 * - faction_ = MONSTER (for hostile NPCs)
	 * - Destructible with hp_ > 0 (can be killed)
	 * - Attacker with power_ > 0 (can deal damage)
	 */
	class Npc : public BaseEntity
	{
	public:
		Npc(pos_t pos, const std::string& name, bool blocker,
		    AttackerComponent attack,
		    const DestructibleComponent& defense,
		    const IconRenderable& renderable, Faction faction,
		    std::unique_ptr<AiComponent> ai, bool pickable = true,
		    bool isCorpse = false);

		void Act(Engine& engine) override;
		std::unique_ptr<AiComponent> SwapAi(
		    std::unique_ptr<AiComponent> newAi);

	private:
		std::unique_ptr<AiComponent> ai_;
	};
} // namespace tutorial

namespace tutorial
{
	/**
	 * Player - Player-controlled entity with inventory
	 *
	 * Required components (in addition to BaseEntity components):
	 * - Inventory (vector of Entity items, always present)
	 *
	 * Component invariants:
	 * - faction_ must be PLAYER
	 * - blocker_ must be true
	 * - Must have Destructible with hp_ > 0
	 * - Must have Attacker (even if power is low)
	 */
	class Player : public BaseEntity
	{
	public:
		Player(pos_t pos, const std::string& name, bool blocker,
		       AttackerComponent attack,
		       const DestructibleComponent& defense,
		       const IconRenderable& renderable, Faction faction,
		       bool pickable = true, bool isCorpse = false);

		void Use(Engine& engine) override;
		bool AddToInventory(std::unique_ptr<Entity> item);
		Entity* GetInventoryItem(size_t index);
		size_t GetInventorySize() const;
		const std::vector<std::unique_ptr<Entity>>& GetInventory()
		    const;
		void RemoveFromInventory(size_t index);
		std::unique_ptr<Entity> ExtractFromInventory(size_t index);

	private:
		std::vector<std::unique_ptr<Entity>> inventory_;
	};
} // namespace tutorial

#endif // ENTITY_HPP
