#ifndef CHARACTER_CREATION_MENU_HPP
#define CHARACTER_CREATION_MENU_HPP

#include "MenuBase.hpp"
#include "MenuWindow.hpp"
#include "Position.hpp"

#include <memory>
#include <string>

namespace tutorial
{
	class Engine;

	// Simple character creation menu - just class selection
	// No tabs, no stats, just pick a class and go
	class CharacterCreationMenu : public MenuBase
	{
	public:
		CharacterCreationMenu(Engine& engine, const std::string& title,
		                      pos_t position, int width, int height);

		void Render(TCOD_Console* console) override;
		bool HandleInput() override;
		BackgroundMode GetBackgroundMode() const override;

	private:
		std::unique_ptr<MenuWindow> window_;
	};

} // namespace tutorial

#endif // CHARACTER_CREATION_MENU_HPP
