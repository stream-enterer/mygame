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

		auto& cfg = ConfigManager::Instance();
		auto frameColor = cfg.GetUIFrameColor();
		auto textColor = cfg.GetUITextColor();

		const int width = TCOD_console_get_width(console_);
		const int height = TCOD_console_get_height(console_);

		// Draw frame with box-drawing characters
		TCOD_console_draw_frame_rgb(console_, 0, 0, width, height, NULL,
		                            NULL, NULL, TCOD_BKGND_DEFAULT,
		                            true);

		// Color the frame
		for (int x = 0; x < width; ++x) {
			TCOD_console_set_char_foreground(console_, x, 0,
			                                 frameColor);
			TCOD_console_set_char_foreground(
			    console_, x, height - 1, frameColor);
		}
		for (int y = 1; y < height - 1; ++y) {
			TCOD_console_set_char_foreground(console_, 0, y,
			                                 frameColor);
			TCOD_console_set_char_foreground(console_, width - 1, y,
			                                 frameColor);
		}

		// Draw title in white on top border (no background)
		int titleX = (width - static_cast<int>(title_.length())) / 2;
		TCOD_printf_rgb(
		    console_,
		    (TCOD_PrintParamsRGB) { .x = titleX,
		                            .y = 0,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &textColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", title_.c_str());

		// Display items with shortcuts
		if (auto* player = dynamic_cast<const Player*>(&player_)) {
			const auto& inventory = player->GetInventory();
			char shortcut = 'a';
			int y = 1;

			for (const auto& item : inventory) {
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
				                item->GetName().c_str());

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

		// Blit to parent: foreground opaque (1.0), background
		// transparent (0.8)
		TCOD_console_blit(console_, 0, 0, width, height, parent, pos_.x,
		                  pos_.y, 1.0f, 0.8f);
	}

} // namespace tutorial
