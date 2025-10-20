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
        AttackerComponent(unsigned int power);

        unsigned int Attack() const;
        void IncreasePower(unsigned int amount);

    private:
        unsigned int power_;
    };

    class DestructibleComponent
    {
    public:
        DestructibleComponent(unsigned int defense, unsigned int hp);
        DestructibleComponent(unsigned int defense, unsigned int maxHp,
                              unsigned int hp);

        unsigned int Heal(unsigned int value);
        void TakeDamage(unsigned int value);

        unsigned int GetDefense() const;
        int GetHealth() const;
        unsigned int GetMaxHealth() const;
        bool IsDead() const;

        // XP management
        unsigned int GetXp() const;
        void AddXp(unsigned int amount);
        unsigned int GetXpReward() const;
        void SetXpReward(unsigned int reward);
        void IncreaseMaxHealth(unsigned int amount);
        void IncreaseDefense(unsigned int amount);

    private:
        unsigned int defense_;
        unsigned int maxHp_;
        int hp_;
        unsigned int xp_;       // Current XP (for player)
        unsigned int xpReward_; // XP granted when this entity dies
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
