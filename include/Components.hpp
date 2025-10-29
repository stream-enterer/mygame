#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "Position.hpp"

#include <libtcod.h>
#include <libtcod/color.hpp>

namespace tutorial
{
	class AttackerComponent
	{
	public:
		AttackerComponent(unsigned int strength);

		unsigned int Attack() const; // Returns STR value
		unsigned int GetStrength() const;
		void IncreaseStrength(unsigned int amount);

	private:
		unsigned int strength_; // STR stat (1 STR = 1 attack damage)
	};

	class DestructibleComponent
	{
	public:
		DestructibleComponent(unsigned int dexterity, unsigned int hp);
		DestructibleComponent(unsigned int dexterity,
		                      unsigned int maxHp, unsigned int hp);

		unsigned int Heal(unsigned int value);
		void TakeDamage(unsigned int value);

		unsigned int GetDefense() const; // Returns DEX value
		unsigned int GetDexterity() const;
		int GetHealth() const;
		unsigned int GetMaxHealth() const;
		bool IsDead() const;

		// XP management
		unsigned int GetXp() const;
		void AddXp(unsigned int amount);
		unsigned int GetXpReward() const;
		void SetXpReward(unsigned int reward);
		void IncreaseMaxHealth(unsigned int amount);
		void IncreaseDexterity(unsigned int amount);

		// Level calculation helpers
		unsigned int CalculateLevel(unsigned int xp) const;
		bool CheckLevelUp(unsigned int oldXp, unsigned int newXp) const;

		// Mana management (MP system - works like HP)
		unsigned int GetMp() const;
		unsigned int GetMaxMp() const;
		void SpendMp(unsigned int amount);
		void RegenerateMp(unsigned int amount);
		unsigned int GetIntelligence() const;
		void IncreaseIntelligence(unsigned int amount);
		void IncreaseMaxMp(unsigned int amount);

	private:
		unsigned int dexterity_; // DEX stat (1 DEX = 1 defense)
		unsigned int maxHp_;
		int hp_;
		unsigned int xp_;       // Current XP (for player)
		unsigned int xpReward_; // XP granted when this entity dies
		unsigned int intelligence_;
		unsigned int mp_;
		unsigned int maxMp_;
	};
	class RenderableComponent
	{
	public:
		virtual ~RenderableComponent() = default;

		// Changed parameter to TCOD_Console* (C API type)
		virtual void Render(TCOD_Console* parent, pos_t pos) const = 0;
	};

	class IconRenderable : public RenderableComponent
	{
	public:
		// Changed color parameter to tcod::ColorRGB (modern type)
		IconRenderable(tcod::ColorRGB color, char icon);

		void Render(TCOD_Console* parent, pos_t pos) const override;

		char GetIcon() const
		{
			return icon_;
		}
		tcod::ColorRGB GetColor() const
		{
			return color_;
		}

	private:
		tcod::ColorRGB color_; // Changed from TCODColor
		char icon_;
	};
} // namespace tutorial

#endif // COMPONENTS_HPP
