#include "Components.hpp"

namespace tutorial
{
	inline namespace
	{
		using uint = unsigned int;
	}

	AttackerComponent::AttackerComponent(uint strength)
	    : strength_(strength)
	{
	}

	uint AttackerComponent::Attack() const
	{
		return strength_; // 1 STR = 1 attack damage
	}

	uint AttackerComponent::GetStrength() const
	{
		return strength_;
	}

	void AttackerComponent::IncreaseStrength(uint amount)
	{
		strength_ += amount;
	}

	DestructibleComponent::DestructibleComponent(uint dexterity, uint hp)
	    : dexterity_(dexterity),
	      maxHp_(hp),
	      hp_(hp),
	      xp_(0),
	      xpReward_(0),
	      intelligence_(1),
	      mana_(1),
	      maxMana_(1)
	{
	}

	DestructibleComponent::DestructibleComponent(uint dexterity, uint maxHp,
	                                             uint hp)
	    : dexterity_(dexterity),
	      maxHp_(maxHp),
	      hp_(hp),
	      xp_(0),
	      xpReward_(0),
	      intelligence_(1),
	      mana_(1),
	      maxMana_(1)
	{
	}

	unsigned int DestructibleComponent::Heal(uint value)
	{
		uint oldHp = hp_;
		hp_ = std::min<uint>(maxHp_, hp_ + value);
		return hp_ - oldHp;
	}

	void DestructibleComponent::TakeDamage(uint value)
	{
		hp_ -= std::max<uint>(0, std::min<uint>(value, maxHp_));
	}

	uint DestructibleComponent::GetDefense() const
	{
		return dexterity_; // 1 DEX = 1 defense
	}

	uint DestructibleComponent::GetDexterity() const
	{
		return dexterity_;
	}

	int DestructibleComponent::GetHealth() const
	{
		return hp_;
	}

	uint DestructibleComponent::GetMaxHealth() const
	{
		return maxHp_;
	}

	bool DestructibleComponent::IsDead() const
	{
		return (hp_ <= 0);
	}

	uint DestructibleComponent::GetXp() const
	{
		return xp_;
	}

	void DestructibleComponent::AddXp(uint amount)
	{
		xp_ += amount;
	}

	uint DestructibleComponent::GetXpReward() const
	{
		return xpReward_;
	}

	void DestructibleComponent::SetXpReward(uint reward)
	{
		xpReward_ = reward;
	}

	void DestructibleComponent::IncreaseMaxHealth(uint amount)
	{
		maxHp_ += amount;
		hp_ += amount; // Also heal by the same amount
	}

	void DestructibleComponent::IncreaseDexterity(uint amount)
	{
		dexterity_ += amount;
	}

	unsigned int DestructibleComponent::GetMana() const
	{
		return mana_;
	}

	unsigned int DestructibleComponent::GetMaxMana() const
	{
		return maxMana_;
	}

	void DestructibleComponent::SpendMana(unsigned int amount)
	{
		if (amount > mana_) {
			mana_ = 0;
		} else {
			mana_ -= amount;
		}
	}

	void DestructibleComponent::RestoreMana(unsigned int amount)
	{
		mana_ = std::min(maxMana_, mana_ + amount);
	}

	unsigned int DestructibleComponent::GetIntelligence() const
	{
		return intelligence_;
	}

	void DestructibleComponent::IncreaseIntelligence(unsigned int amount)
	{
		intelligence_ += amount;
		maxMana_ += amount; // Each INT point = +1 max mana
		mana_ += amount;    // Also restore mana by the same amount
	}

	unsigned int DestructibleComponent::CalculateLevel(unsigned int xp) const
	{
		const unsigned int LEVEL_UP_BASE = 200;
		const unsigned int LEVEL_UP_FACTOR = 150;

		unsigned int level = 1;
		unsigned int xpForCurrentLevel = 0;

		while (xpForCurrentLevel + LEVEL_UP_BASE + level * LEVEL_UP_FACTOR
		       <= xp) {
			xpForCurrentLevel += LEVEL_UP_BASE + level * LEVEL_UP_FACTOR;
			level++;
		}

		return level;
	}

	bool DestructibleComponent::CheckLevelUp(unsigned int oldXp,
	                                         unsigned int newXp) const
	{
		unsigned int oldLevel = CalculateLevel(oldXp);
		unsigned int newLevel = CalculateLevel(newXp);
		return newLevel > oldLevel;
	}

	IconRenderable::IconRenderable(tcod::ColorRGB color, char icon)
	    : color_(color), icon_(icon)
	{
	}

	void IconRenderable::Render(TCOD_Console* parent, pos_t pos) const
	{
		TCOD_console_put_rgb(parent, pos.x, pos.y, icon_, &color_, NULL,
		                     TCOD_BKGND_NONE);
	}
} // namespace tutorial
