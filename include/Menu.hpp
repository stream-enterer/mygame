#ifndef MENU_HPP
#define MENU_HPP

#include <SDL3/SDL.h>
#include <libtcod.h>

#include <functional>
#include <string>
#include <vector>

namespace tutorial
{
	class Engine;

	// Single menu item
	struct MenuItem {
		std::string label;
		std::function<void(Engine&)> onSelect;
	};

	// Minimal menu - just a list of items
	class Menu
	{
	public:
		Menu() : selectedIndex_(0), allowEscape_(true)
		{
		}

		void AddItem(const std::string& label,
		             std::function<void(Engine&)> onSelect);

		void Render(TCOD_Console* console, int x, int y) const;

		// Returns true if menu should close
		bool HandleInput(SDL_Keycode key, char character,
		                 Engine& engine);

		void SetAllowEscape(bool allow)
		{
			allowEscape_ = allow;
		}

	private:
		std::vector<MenuItem> items_;
		int selectedIndex_;
		bool allowEscape_;
	};

} // namespace tutorial

#endif // MENU_HPP
