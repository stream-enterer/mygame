#include "Menu.hpp"
#include "Engine.hpp"

namespace tutorial
{
	void Menu::AddItem(const std::string& label,
	                   std::function<void(Engine&)> onSelect)
	{
		items_.push_back({ label, std::move(onSelect) });
	}

	void Menu::Render(TCOD_Console* console, int x, int y) const
	{
		for (size_t i = 0; i < items_.size(); ++i) {
			bool selected = (static_cast<int>(i) == selectedIndex_);
			char letter = 'a' + static_cast<char>(i);

			TCOD_ColorRGB color =
			    selected ? TCOD_ColorRGB { 255, 255, 0 }
			             : TCOD_ColorRGB { 150, 150, 150 };

			TCOD_console_set_default_foreground(console, color);

			// Format: "(a) Item Name"
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "(%c) %s", letter,
			         items_[i].label.c_str());
			TCOD_console_print(console, x, y + i, buffer);
		}

		// Reset color
		TCOD_console_set_default_foreground(
		    console, TCOD_ColorRGB { 255, 255, 255 });
	}

	bool Menu::HandleInput(SDL_Keycode key, char character, Engine& engine)
	{
		// Up arrow
		if (key == SDLK_UP) {
			selectedIndex_--;
			if (selectedIndex_ < 0) {
				selectedIndex_ = items_.size() - 1;
			}
			return false;
		}

		// Down arrow
		if (key == SDLK_DOWN) {
			selectedIndex_++;
			if (selectedIndex_ >= static_cast<int>(items_.size())) {
				selectedIndex_ = 0;
			}
			return false;
		}

		// Enter - select current item
		if (key == SDLK_RETURN) {
			if (selectedIndex_ >= 0
			    && selectedIndex_
			           < static_cast<int>(items_.size())) {
				items_[selectedIndex_].onSelect(engine);
				return true;
			}
		}

		// Letter key - direct selection
		if (character >= 'a'
		    && character < 'a' + static_cast<char>(items_.size())) {
			int index = character - 'a';
			items_[index].onSelect(engine);
			return true;
		}

		// Escape - close menu if allowed
		if (key == SDLK_ESCAPE && allowEscape_) {
			return true;
		}

		return false;
	}

} // namespace tutorial
