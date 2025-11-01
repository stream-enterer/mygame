#include "SpellMenuWindow.hpp"

#include "Colors.hpp"
#include "ConfigManager.hpp"
#include "Entity.hpp"
#include "SpellRegistry.hpp"
#include "SpellcasterComponent.hpp"

namespace tutorial
{
	SpellMenuWindow::SpellMenuWindow(std::size_t width, std::size_t height,
	                                 pos_t pos, const Entity& player)
	    : UiWindowBase(width, height, pos), player_(player)
	{
	}

	void SpellMenuWindow::Render(TCOD_Console* parent) const
	{
		TCOD_console_clear(console_);

		// Draw border
		auto frameColor = ConfigManager::Instance().GetUIFrameColor();
		DrawBorder(console_, frameColor);

		// Draw title
		std::string title = "Cast which spell?";
		int titleX = (TCOD_console_get_width(console_)
		              - static_cast<int>(title.length()))
		             / 2;
		TCOD_printf_rgb(
		    console_,
		    (TCOD_PrintParamsRGB) { .x = titleX,
		                            .y = 0,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &frameColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", title.c_str());

		// Display spells with shortcuts
		auto* caster = player_.GetSpellcaster();
		if (!caster) {
			// Player has no spellcaster component
			auto textColor =
			    ConfigManager::Instance().GetUITextColor();
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = 2,
			                            .y = 1,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", "(you cannot cast spells)");
		} else {
			auto textColor =
			    ConfigManager::Instance().GetUITextColor();
			const auto& spells = caster->GetKnownSpells();

			char shortcut = 'a';
			int y = 1;

			for (const auto& spellId : spells) {
				const SpellData* spell =
				    SpellRegistry::Instance().Get(spellId);
				if (!spell) {
					continue;
				}

				// Get current MP
				auto* destructible = player_.GetDestructible();
				unsigned int currentMp =
				    destructible ? destructible->GetMp() : 0;

				// Format: (a) Fireball (3 MP)
				char buffer[128];
				snprintf(buffer, sizeof(buffer),
				         "(%c) %s (%u MP)", shortcut,
				         spell->name.c_str(), spell->manaCost);

				// Gray out if not enough MP
				tcod::ColorRGB color = textColor;
				if (currentMp < spell->manaCost) {
					color =
					    tcod::ColorRGB { 128, 128, 128 };
				}

				TCOD_printf_rgb(console_,
				                (TCOD_PrintParamsRGB) {
				                    .x = 2,
				                    .y = y,
				                    .width = 0,
				                    .height = 0,
				                    .fg = &color,
				                    .bg = NULL,
				                    .flag = TCOD_BKGND_NONE,
				                    .alignment = TCOD_LEFT },
				                "%s", buffer);

				y++;
				shortcut++;
			}

			if (spells.empty()) {
				TCOD_printf_rgb(console_,
				                (TCOD_PrintParamsRGB) {
				                    .x = 2,
				                    .y = 1,
				                    .width = 0,
				                    .height = 0,
				                    .fg = &textColor,
				                    .bg = NULL,
				                    .flag = TCOD_BKGND_NONE,
				                    .alignment = TCOD_LEFT },
				                "%s", "(no spells known)");
			}
		}

		// Blit to parent
		TCOD_console_blit(console_, 0, 0,
		                  TCOD_console_get_width(console_),
		                  TCOD_console_get_height(console_), parent,
		                  pos_.x, pos_.y, 1.0f, 1.0f);
	}
} // namespace tutorial
