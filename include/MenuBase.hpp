#ifndef MENU_BASE_HPP
#define MENU_BASE_HPP

#include <libtcod.h>

namespace tutorial
{
	class Engine;

	// Specifies what should be rendered behind a menu
	enum class BackgroundMode {
		None,            // Full-screen menu - nothing behind (StartMenu, CharacterCreation)
		DimmedGameWorld, // Overlay with dimmed game view (PauseMenu, LevelUpMenu)
		GameWorld        // Transparent overlay with full game view (Inventory, ItemSelection, MessageHistory)
	};

	// Abstract base class for all menu types
	// Defines the interface that all menus must implement
	class MenuBase
	{
	public:
		virtual ~MenuBase() = default;

		// Render this menu to the console
		virtual void Render(TCOD_Console* console) = 0;

		// Handle input and return true if menu should stay active
		// Return false to close/pop the menu
		virtual bool HandleInput() = 0;

		// What to render behind this menu
		virtual BackgroundMode GetBackgroundMode() const = 0;

		// Should this menu pause the game?
		virtual bool PausesGame() const
		{
			return true;
		}

	protected:
		Engine& engine_;
		explicit MenuBase(Engine& engine) : engine_(engine) {}
	};

} // namespace tutorial

#endif // MENU_BASE_HPP
