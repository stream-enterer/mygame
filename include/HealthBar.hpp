#ifndef HEALTH_BAR_HPP
#define HEALTH_BAR_HPP

#include "Entity.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

namespace tutorial
{
    class HealthBar : public UiWindowBase
    {
    public:
        HealthBar(unsigned int width, unsigned int height, pos_t pos,
                  const Entity& entity);

        void Render(TCOD_Console* parent) const override;

    private:
        const Entity& entity_;
    };
} // namespace tutorial

#endif // HEALTH_BAR_HPP
