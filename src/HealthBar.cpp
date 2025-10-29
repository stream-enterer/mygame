#include "HealthBar.hpp"

#include "Colors.hpp"
#include "Components.hpp"
#include "ConfigManager.hpp"

namespace tutorial
{
	HealthBar::HealthBar(unsigned int width, unsigned int height, pos_t pos,
	                     const Entity& entity)
	    : UiWindowBase(width, height, pos), entity_(entity)
	{
		// Fill the background with empty health bar color from config
		tcod::ColorRGB emptyColor =
		    ConfigManager::Instance().GetHealthBarEmptyColor();

		// Fill the background with empty color
		TCOD_console_draw_rect_rgb(console_, 0, 0,
		                           TCOD_console_get_width(console_),
		                           TCOD_console_get_height(console_), 0,
		                           NULL, &emptyColor, TCOD_BKGND_SET);
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

		if (destructible != nullptr) {
			const int consoleWidth =
			    TCOD_console_get_width(console_);

			// === HP BAR (Row 0) ===
			// Fill background with empty HP color
			tcod::ColorRGB hpEmpty =
			    ConfigManager::Instance().GetHealthBarEmptyColor();
			for (int i = 0; i < consoleWidth; ++i) {
				TCOD_console_put_rgb(console_, i, 0, 0, NULL,
				                     &hpEmpty, TCOD_BKGND_SET);
			}

			// Fill foreground with full HP color
			const int hpWidth =
			    (int)((float)(destructible->GetHealth())
			          / destructible->GetMaxHealth()
			          * consoleWidth);
			if (hpWidth > 0) {
				tcod::ColorRGB hpFull =
				    ConfigManager::Instance()
				        .GetHealthBarFullColor();
				for (int i = 0; i < hpWidth; ++i) {
					TCOD_console_put_rgb(console_, i, 0, 0,
					                     NULL, &hpFull,
					                     TCOD_BKGND_SET);
				}
			}

			// Print HP text
			char hpBuffer[50];
			snprintf(hpBuffer, sizeof(hpBuffer), "HP: %i/%i",
			         destructible->GetHealth(),
			         destructible->GetMaxHealth());
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = 1,
			                            .y = 0,
			                            .width = 0,
			                            .height = 0,
			                            .fg = NULL,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", hpBuffer);

			// === MANA BAR (Row 1) ===
			// Fill background with empty mana color
			tcod::ColorRGB manaEmpty =
			    ConfigManager::Instance().GetManaBarEmptyColor();
			for (int i = 0; i < consoleWidth; ++i) {
				TCOD_console_put_rgb(console_, i, 1, 0, NULL,
				                     &manaEmpty,
				                     TCOD_BKGND_SET);
			}

			// Fill foreground with full mana color
			const int manaWidth =
			    (int)((float)destructible->GetMp()
			          / destructible->GetMaxMp() * consoleWidth);
			if (manaWidth > 0) {
				tcod::ColorRGB manaFull =
				    ConfigManager::Instance()
				        .GetManaBarFullColor();
				for (int i = 0; i < manaWidth; ++i) {
					TCOD_console_put_rgb(console_, i, 1, 0,
					                     NULL, &manaFull,
					                     TCOD_BKGND_SET);
				}
			}

			// Print mana text
			char manaBuffer[50];
			snprintf(manaBuffer, sizeof(manaBuffer), "MP: %u/%u",
			         destructible->GetMp(),
			         destructible->GetMaxMp());
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = 1,
			                            .y = 1,
			                            .width = 0,
			                            .height = 0,
			                            .fg = NULL,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", manaBuffer);

			// === XP BAR (Row 3) ===
			// Calculate XP level from current XP
			unsigned int currentXp = destructible->GetXp();
			unsigned int xpLevel = 1;
			unsigned int xpForCurrentLevel = 0;

			// Determine which level the player is at
			while (xpForCurrentLevel + GetNextLevelXp(xpLevel)
			       <= currentXp) {
				xpForCurrentLevel += GetNextLevelXp(xpLevel);
				xpLevel++;
			}

			unsigned int xpIntoCurrentLevel =
			    currentXp - xpForCurrentLevel;
			unsigned int xpNeededForNextLevel =
			    GetNextLevelXp(xpLevel);

			// Fill background with empty XP color
			tcod::ColorRGB xpEmpty =
			    ConfigManager::Instance().GetXpBarEmptyColor();
			for (int i = 0; i < consoleWidth; ++i) {
				TCOD_console_put_rgb(console_, i, 3, 0, NULL,
				                     &xpEmpty, TCOD_BKGND_SET);
			}

			// Fill foreground with full XP color
			const int xpWidth =
			    (int)((float)xpIntoCurrentLevel
			          / xpNeededForNextLevel * consoleWidth);
			if (xpWidth > 0) {
				tcod::ColorRGB xpFull =
				    ConfigManager::Instance()
				        .GetXpBarFullColor();
				for (int i = 0; i < xpWidth; ++i) {
					TCOD_console_put_rgb(console_, i, 3, 0,
					                     NULL, &xpFull,
					                     TCOD_BKGND_SET);
				}
			}

			// Print XP text
			char xpBuffer[50];
			snprintf(xpBuffer, sizeof(xpBuffer),
			         "XP: %u/%u (Lvl %u)", xpIntoCurrentLevel,
			         xpNeededForNextLevel, xpLevel);
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = 1,
			                            .y = 3,
			                            .width = 0,
			                            .height = 0,
			                            .fg = NULL,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", xpBuffer);

			// === STAT DISPLAY (Row 4) ===
			// Get stats from components
			unsigned int str = 0;
			unsigned int dex = 0;
			unsigned int intel = 0;

			if (auto* attacker = entity_.GetAttacker()) {
				str = attacker->GetStrength();
			}
			dex = destructible->GetDexterity();
			intel = destructible->GetIntelligence();

			// Print stats
			char statBuffer[50];
			snprintf(statBuffer, sizeof(statBuffer),
			         "STR:%u DEX:%u INT:%u", str, dex, intel);

			tcod::ColorRGB statColor =
			    ConfigManager::Instance().GetUITextColor();

			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = 1,
			                            .y = 4,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &statColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", statBuffer);
		}

		TCOD_console_blit(console_, 0, 0,
		                  TCOD_console_get_width(console_),
		                  TCOD_console_get_height(console_), parent,
		                  pos_.x, pos_.y, 1.0f, 1.0f);
	}
} // namespace tutorial
