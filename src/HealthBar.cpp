#include "HealthBar.hpp"

#include "Colors.hpp"
#include "Components.hpp"
#include "ConfigManager.hpp"

namespace tutorial
{
    HealthBar::HealthBar(unsigned int width, unsigned int height, pos_t pos,
                         const Entity& entity) :
        UiWindowBase(width, height, pos), entity_(entity)
    {
        // Fill the background with empty health bar color from config
        tcod::ColorRGB emptyColor =
            ConfigManager::Instance().GetHealthBarEmptyColor();

        TCOD_console_rect(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), true,
                          TCOD_BKGND_SET);

        for (int x = 0; x < TCOD_console_get_width(console_); ++x)
        {
            for (int y = 0; y < TCOD_console_get_height(console_); ++y)
            {
                TCOD_console_set_char_background(console_, x, y, emptyColor,
                                                 TCOD_BKGND_SET);
            }
        }
    }

    unsigned int HealthBar::GetNextLevelXp(unsigned int currentLevel) const
    {
        // XP formula: 200 base + 150 per level
        // Level 1->2: 350 XP, Level 2->3: 500 XP, etc.
        const unsigned int LEVEL_UP_BASE = 200;
        const unsigned int LEVEL_UP_FACTOR = 150;
        return LEVEL_UP_BASE + currentLevel * LEVEL_UP_FACTOR;
    }

    void HealthBar::Render(TCOD_Console* parent) const
    {
        TCOD_console_clear(console_);

        auto destructible = entity_.GetDestructible();

        if (destructible != nullptr)
        {
            const int consoleWidth = TCOD_console_get_width(console_);

            // === HP BAR (Row 0) ===
            // Fill background with empty HP color
            tcod::ColorRGB hpEmpty =
                ConfigManager::Instance().GetHealthBarEmptyColor();
            for (int i = 0; i < consoleWidth; ++i)
            {
                TCOD_console_set_char_background(console_, i, 0, hpEmpty,
                                                 TCOD_BKGND_SET);
            }

            // Fill foreground with full HP color
            const int hpWidth =
                (int)((float)(destructible->GetHealth())
                      / destructible->GetMaxHealth() * consoleWidth);
            if (hpWidth > 0)
            {
                tcod::ColorRGB hpFull =
                    ConfigManager::Instance().GetHealthBarFullColor();
                for (int i = 0; i < hpWidth; ++i)
                {
                    TCOD_console_set_char_background(console_, i, 0, hpFull,
                                                     TCOD_BKGND_SET);
                }
            }

            // Print HP text
            char hpBuffer[50];
            snprintf(hpBuffer, sizeof(hpBuffer), "HP: %i/%i",
                     destructible->GetHealth(), destructible->GetMaxHealth());
            TCOD_console_print(console_, 1, 0, "%s", hpBuffer);

            // === XP BAR (Row 2) ===
            // Calculate XP level from current XP
            unsigned int currentXp = destructible->GetXp();
            unsigned int xpLevel = 1;
            unsigned int xpForCurrentLevel = 0;

            // Determine which level the player is at
            while (xpForCurrentLevel + GetNextLevelXp(xpLevel) <= currentXp)
            {
                xpForCurrentLevel += GetNextLevelXp(xpLevel);
                xpLevel++;
            }

            unsigned int xpIntoCurrentLevel = currentXp - xpForCurrentLevel;
            unsigned int xpNeededForNextLevel = GetNextLevelXp(xpLevel);

            // Fill background with empty XP color
            tcod::ColorRGB xpEmpty =
                ConfigManager::Instance().GetXpBarEmptyColor();
            for (int i = 0; i < consoleWidth; ++i)
            {
                TCOD_console_set_char_background(console_, i, 2, xpEmpty,
                                                 TCOD_BKGND_SET);
            }

            // Fill foreground with full XP color
            const int xpWidth = (int)((float)xpIntoCurrentLevel
                                      / xpNeededForNextLevel * consoleWidth);
            if (xpWidth > 0)
            {
                tcod::ColorRGB xpFull =
                    ConfigManager::Instance().GetXpBarFullColor();
                for (int i = 0; i < xpWidth; ++i)
                {
                    TCOD_console_set_char_background(console_, i, 2, xpFull,
                                                     TCOD_BKGND_SET);
                }
            }

            // Print XP text
            char xpBuffer[50];
            snprintf(xpBuffer, sizeof(xpBuffer), "XP: %u/%u (Lvl %u)",
                     xpIntoCurrentLevel, xpNeededForNextLevel, xpLevel);
            TCOD_console_print(console_, 1, 2, "%s", xpBuffer);
        }

        TCOD_console_blit(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), parent, pos_.x,
                          pos_.y, 1.0f, 1.0f);
    }
} // namespace tutorial
