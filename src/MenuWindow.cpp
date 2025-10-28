#include "MenuWindow.hpp"

#include "ConfigManager.hpp"

#include <algorithm>

namespace tutorial
{
	MenuWindow::MenuWindow(std::size_t width, std::size_t height, pos_t pos,
	                       const std::string& title, bool fullScreenBorder)
	    : UiWindowBase(width, height, pos),
	      title_(title),
	      selectedIndex_(0),
	      markedIndex_(-1),
	      fullScreenBorder_(fullScreenBorder),
	      showLetters_(false),
	      showMarker_(false),
	      gameLogoStub_("")
	{
	}

	void MenuWindow::Render(TCOD_Console* parent) const
	{
		if (!console_ || items_.empty()) {
			return;
		}

		auto& cfg = ConfigManager::Instance();
		auto frameColor = cfg.GetUIFrameColor();
		auto textColor = cfg.GetUITextColor();

		// Clear console
		TCOD_console_clear(console_);

		// Draw frame (always at edges of the console)
		const int width = TCOD_console_get_width(console_);
		const int height = TCOD_console_get_height(console_);

		// Draw corners
		TCOD_console_put_rgb(console_, 0, 0, 0x2219, &frameColor, NULL,
		                     TCOD_BKGND_SET); // Top-left: ∙
		TCOD_console_put_rgb(console_, width - 1, 0, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Top-right: ∙
		TCOD_console_put_rgb(console_, 0, height - 1, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Bottom-left: ∙
		TCOD_console_put_rgb(console_, width - 1, height - 1, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Bottom-right: ∙

		// Draw horizontal edges
		for (int x = 1; x < width - 1; ++x) {
			TCOD_console_put_rgb(console_, x, 0, 0x2550,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Top: ═
			TCOD_console_put_rgb(console_, x, height - 1, 0x2550,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Bottom: ═
		}

		// Draw vertical edges
		for (int y = 1; y < height - 1; ++y) {
			TCOD_console_put_rgb(console_, 0, y, 0x2551,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Left: ║
			TCOD_console_put_rgb(console_, width - 1, y, 0x2551,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Right: ║
		}

		// Draw game logo stub if present
		int contentStartY = 2;
		if (!gameLogoStub_.empty()) {
			int logoX =
			    (width - static_cast<int>(gameLogoStub_.length()))
			    / 2;
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = logoX,
			                            .y = contentStartY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", gameLogoStub_.c_str());
			contentStartY += 3;
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

		// Draw menu items (adjust starting position)
		int startY = contentStartY;
		if (!gameLogoStub_.empty()) {
			startY += 3; // Space after logo
		} else {
			startY += 1; // Space after title on border
		}
		for (size_t i = 0; i < items_.size(); ++i) {
			tcod::ColorRGB itemColor;
			if (static_cast<int>(i) == selectedIndex_) {
				// Highlight selected item
				itemColor =
				    tcod::ColorRGB { 255, 200,
					             100 }; // Light orange
			} else {
				itemColor = textColor;
			}

			int itemY = startY + static_cast<int>(i) * 2;

			// Build the item string with optional letter prefix and
			// asterisk
			std::string itemText;
			if (showMarker_
			    && static_cast<int>(i) == markedIndex_) {
				itemText += "* ";
			} else if (showMarker_) {
				itemText += "  ";
			}

			if (showLetters_) {
				char letter = 'a' + static_cast<char>(i);
				itemText += "(";
				itemText += letter;
				itemText += ") ";
			}

			itemText += items_[i].label;

			int itemX =
			    (width - static_cast<int>(itemText.length())) / 2;
			TCOD_printf_rgb(
			    console_,
			    (TCOD_PrintParamsRGB) { .x = itemX,
			                            .y = itemY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &itemColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", itemText.c_str());
		}

		// Blit to parent
		float bgAlpha = fullScreenBorder_ ? 1.0f : 0.8f;
		TCOD_console_blit(console_, 0, 0, width, height, parent, pos_.x,
		                  pos_.y, 1.0f, bgAlpha);
	}

	void MenuWindow::Clear()
	{
		items_.clear();
		selectedIndex_ = 0;
		markedIndex_ = -1;
	}

	void MenuWindow::AddItem(MenuAction action, const std::string& label)
	{
		items_.push_back({ action, label });
	}

	void MenuWindow::SelectPrevious()
	{
		if (items_.empty()) {
			return;
		}

		selectedIndex_--;
		if (selectedIndex_ < 0) {
			selectedIndex_ = static_cast<int>(items_.size()) - 1;
		}
	}

	void MenuWindow::SelectNext()
	{
		if (items_.empty()) {
			return;
		}

		selectedIndex_ =
		    (selectedIndex_ + 1) % static_cast<int>(items_.size());
	}

	MenuAction MenuWindow::GetSelectedAction() const
	{
		if (selectedIndex_ >= 0
		    && selectedIndex_ < static_cast<int>(items_.size())) {
			return items_[selectedIndex_].action;
		}
		return MenuAction::None;
	}

	bool MenuWindow::SelectByLetter(char letter)
	{
		if (!showLetters_ || items_.empty()) {
			return false;
		}

		// Convert to lowercase
		if (letter >= 'A' && letter <= 'Z') {
			letter = letter - 'A' + 'a';
		}

		// Calculate index from letter
		int index = letter - 'a';
		if (index >= 0 && index < static_cast<int>(items_.size())) {
			selectedIndex_ = index;
			return true;
		}

		return false;
	}

	void MenuWindow::MarkCurrentSelection()
	{
		markedIndex_ = selectedIndex_;
	}

} // namespace tutorial
