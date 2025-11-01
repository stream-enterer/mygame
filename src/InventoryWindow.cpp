#include "InventoryWindow.hpp"

#include "Colors.hpp"
#include "ConfigManager.hpp"
#include "Engine.hpp"
#include "Entity.hpp"

namespace tutorial
{
	InventoryWindow::InventoryWindow(std::size_t width, std::size_t height,
	                                 pos_t pos, const Entity& player)
	    : UiWindowBase(width, height, pos),
	      title_("Inventory"),
	      player_(player)
	{
	}

	void InventoryWindow::Render(TCOD_Console* parent) const
	{
		TCOD_console_clear(console_);

		// Draw border
		auto& cfg = ConfigManager::Instance();
		auto frameColor = cfg.GetUIFrameColor();
		DrawBorder(console_, frameColor);

		// Draw title
		int titleX = (TCOD_console_get_width(console_)
		              - static_cast<int>(title_.length()))
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
		    "%s", title_.c_str());

		// Display items with shortcuts
		if (auto* player = dynamic_cast<const Player*>(&player_)) {
			tcod::ColorRGB textColor =
			    tcod::ColorRGB { 255, 255, 255 };

			const auto& inventory = player->GetInventory();
			char shortcut = 'a';
			int y = 1;

			for (const auto& item : inventory) {
				int stackCount = item->GetStackCount();

				std::string displayName;
				if (stackCount > 1) {
					// Show count and plural name for stacks
					displayName = std::to_string(stackCount)
					              + " "
					              + item->GetPluralName();
				} else {
					// Single item, use regular name
					displayName = item->GetName();
				}

				TCOD_printf_rgb(console_,
				                (TCOD_PrintParamsRGB) {
				                    .x = 2,
				                    .y = y,
				                    .width = 0,
				                    .height = 0,
				                    .fg = &textColor,
				                    .bg = NULL,
				                    .flag = TCOD_BKGND_NONE,
				                    .alignment = TCOD_LEFT },
				                "(%c) %s", shortcut,
				                displayName.c_str());

				y++;
				shortcut++;
			}

			if (inventory.empty()) {
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
				                "%s", "(empty)");
			}
		}

		// Blit to parent
		TCOD_console_blit(console_, 0, 0,
		                  TCOD_console_get_width(console_),
		                  TCOD_console_get_height(console_), parent,
		                  pos_.x, pos_.y, 1.0f, 1.0f);
	}
} // namespace tutorial
