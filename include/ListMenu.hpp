#ifndef LIST_MENU_HPP
#define LIST_MENU_HPP

#include "MenuBase.hpp"
#include "MenuWindow.hpp"
#include "Position.hpp"

#include <memory>
#include <string>

namespace tutorial
{
	class Engine;

	// Concrete menu implementation for simple list-based menus
	// Wraps MenuWindow for rendering and adds MenuBase interface
	// Used for: PauseMenu, LevelUpMenu, StartMenu, NewGameConfirmation
	class ListMenu : public MenuBase
	{
	public:
		ListMenu(Engine& engine, const std::string& title, pos_t position,
		         int width, int height, BackgroundMode bgMode,
		         bool fullScreenBorder = false);

		// Add a menu item
		void AddItem(MenuAction action, const std::string& label);

		// Clear all menu items
		void Clear();

		// Set optional game logo stub for start menu
		void SetGameLogoStub(const std::string& logo);

		// MenuBase interface implementation
		void Render(TCOD_Console* console) override;
		bool HandleInput() override;
		BackgroundMode GetBackgroundMode() const override;

	private:
		std::unique_ptr<MenuWindow> window_;
		BackgroundMode backgroundMode_;
	};

} // namespace tutorial

#endif // LIST_MENU_HPP
