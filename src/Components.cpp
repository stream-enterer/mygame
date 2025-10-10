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

    DestructibleComponent::DestructibleComponent(uint defense, uint hp) :
        defense_(defense), maxHp_(hp), hp_(hp)
    {
    }

    DestructibleComponent::DestructibleComponent(uint defense, uint maxHp,
                                                 uint hp) :
        defense_(defense), maxHp_(maxHp), hp_(hp)
    {
    }

    unsigned int DestructibleComponent::Heal(uint value)
    {
        uint oldHp = hp_;
        hp_ += std::min<uint>(maxHp_, hp_ + value);
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

    IconRenderable::IconRenderable(tcod::ColorRGB color, char icon) :
        color_(color), icon_(icon)
    {
    }

    void IconRenderable::Render(TCOD_Console* parent, pos_t pos) const
    {
        // Use C API functions instead of deprecated C++ methods
        // Set the character at this position
        TCOD_console_put_char(parent, pos.x, pos.y, icon_, TCOD_BKGND_NONE);

        // Set the foreground color for this character
        TCOD_console_set_char_foreground(parent, pos.x, pos.y, color_);
    }
} // namespace tutorial
