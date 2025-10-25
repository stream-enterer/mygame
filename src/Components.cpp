#include "Components.hpp"

namespace tutorial
{
	inline namespace
	{
		using uint = unsigned int;
	}

	AttackerComponent::AttackerComponent(uint power) : power_(power)
	{
	}

	uint AttackerComponent::Attack() const
	{
		return power_;
	}

	DestructibleComponent::DestructibleComponent(uint defense, uint hp)
	    : defense_(defense), maxHp_(hp), hp_(hp), xp_(0), xpReward_(0)
	{
	}

	DestructibleComponent::DestructibleComponent(uint defense, uint maxHp,
	                                             uint hp)
	    : defense_(defense), maxHp_(maxHp), hp_(hp), xp_(0), xpReward_(0)
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
		return defense_;
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

	void DestructibleComponent::IncreaseDefense(uint amount)
	{
		defense_ += amount;
	}

	void AttackerComponent::IncreasePower(uint amount)
	{
		power_ += amount;
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
