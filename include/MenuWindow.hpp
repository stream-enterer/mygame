#ifndef MENU_WINDOW_HPP
#define MENU_WINDOW_HPP

#include "Position.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace tutorial
{
	// Menu actions returned when user selects an option
	enum class MenuAction {
		None, // No selection (window closed or ESC pressed)
		NewGame,
		Continue,
		SaveAndQuit,
		Quit,
		LevelUpStrength,     // +1 attack power
		LevelUpDexterity,    // +1 defense (renamed from Agility)
		LevelUpIntelligence, // +1 INT and max mana
		CharacterClass1,     // Warrior class
		CharacterClass2,     // Rogue class
		CharacterClass3,     // Mage class
		ConfirmYes,
		ConfirmNo
	};

	// Individual menu item with label and action
	struct MenuItem {
		MenuAction action;
		std::string label;
	};

	// Window for displaying menus (start menu, pause menu)
	class MenuWindow : public UiWindowBase
	{
	public:
		MenuWindow(std::size_t width, std::size_t height, pos_t pos,
		           const std::string& title);

		void Render(TCOD_Console* parent) const override;

		// Build menu options
		void Clear();
		void AddItem(MenuAction action, const std::string& label);

		// Get menu options for rendering
		const std::vector<MenuItem>& GetItems() const
		{
			return items_;
		}

		// Get currently selected item index
		int GetSelectedIndex() const
		{
			return selectedIndex_;
		}

		// Navigation
		void SelectPrevious();
		void SelectNext();

		// Get the action for currently selected item
		MenuAction GetSelectedAction() const;

	private:
		std::string title_;
		std::vector<MenuItem> items_;
		int selectedIndex_;
	};

} // namespace tutorial

#endif // MENU_WINDOW_HPP
