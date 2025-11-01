#include "ItemSelectionWindow.hpp"

#include "Colors.hpp"
#include "ConfigManager.hpp"
#include "Entity.hpp"

namespace tutorial
{
	ItemSelectionWindow::ItemSelectionWindow(
	    std::size_t width, std::size_t height, pos_t pos,
	    const std::vector<Entity*>& items, const std::string& title)
	    : UiWindowBase(width, height, pos), items_(items), title_(title)
	{
	}

	void ItemSelectionWindow::Render(TCOD_Console* parent) const
	{
		TCOD_console_clear(console_);

		// Draw border
		auto frameColor = ConfigManager::Instance().GetUIFrameColor();
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
		auto textColor = ConfigManager::Instance().GetUITextColor();

		char shortcut = 'a';
		int y = 1;

		for (const auto* item : items_) {
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = 2,
			                            .y = y,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "(%c) %s", shortcut, item->GetName().c_str());
			y++;
			shortcut++;
		}

		if (items_.empty()) {
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
			    "%s", "(nothing here)");
		}

		// Blit to parent
		TCOD_console_blit(console_, 0, 0,
		                  TCOD_console_get_width(console_),
		                  TCOD_console_get_height(console_), parent,
		                  pos_.x, pos_.y, 1.0f, 1.0f);
	}
} // namespace tutorial
