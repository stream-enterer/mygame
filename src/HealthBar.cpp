#include "HealthBar.hpp"

#include "Colors.hpp"
#include "Components.hpp"

namespace tutorial
{
    HealthBar::HealthBar(unsigned int width, unsigned int height, pos_t pos,
                         const Entity& entity)
        : UiWindowBase(width, height, pos), entity_(entity)
    {
        // Fill the background with dark red
        TCOD_console_rect(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), true,
                          TCOD_BKGND_SET);

        for (int x = 0; x < TCOD_console_get_width(console_); ++x)
        {
            for (int y = 0; y < TCOD_console_get_height(console_); ++y)
            {
                TCOD_console_set_char_background(
                    console_, x, y, color::dark_red, TCOD_BKGND_SET);
            }
        }
    }

    void HealthBar::Render(TCOD_Console* parent) const
    {
        TCOD_console_clear(console_);

        auto health = entity_.GetDestructible();

        if (health != nullptr)
        {
            const auto width =
                (int)((float)(health->GetHealth()) / health->GetMaxHealth()
                      * TCOD_console_get_width(console_));

            if (width > 0)
            {
                for (int i = 0; i < width; ++i)
                {
                    TCOD_console_set_char_background(
                        console_, i, TCOD_console_get_height(console_) - 1,
                        color::green, TCOD_BKGND_SET);
                }
            }

            // Print health text
            char buffer[50];
            snprintf(buffer, sizeof(buffer), "HP: %i/%i", health->GetHealth(),
                     health->GetMaxHealth());
            TCOD_console_print(console_, 1, 0, "%s", buffer);
        }

        TCOD_console_blit(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), parent, pos_.x,
                          pos_.y, 1.0f, 1.0f);
    }
} // namespace tutorial
