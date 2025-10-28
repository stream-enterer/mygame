#ifndef MENU_FACTORY_HPP
#define MENU_FACTORY_HPP

#include "MenuBase.hpp"
#include "Position.hpp"

#include <memory>

namespace tutorial
{
	class Engine;

	// Factory class for creating pre-configured menus
	// Centralizes all menu configuration logic
	class MenuFactory
	{
	public:
		// Create a pause menu (shown during gameplay)
		static std::unique_ptr<MenuBase> CreatePauseMenu(Engine& engine);

		// Create a start menu (first screen shown)
		static std::unique_ptr<MenuBase>
		CreateStartMenu(Engine& engine);

		// Create a level-up menu (shown when player levels up)
		static std::unique_ptr<MenuBase>
		CreateLevelUpMenu(Engine& engine);

		// Create a new game confirmation dialog
		static std::unique_ptr<MenuBase>
		CreateNewGameConfirmation(Engine& engine);

		// Create a character creation menu
		static std::unique_ptr<MenuBase>
		CreateCharacterCreation(Engine& engine);

	private:
		// Helper to calculate centered position
		static pos_t CalculateCenteredPosition(int width, int height,
		                                       const Engine& engine);
	};

} // namespace tutorial

#endif // MENU_FACTORY_HPP
