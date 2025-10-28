#ifndef MENU_STACK_HPP
#define MENU_STACK_HPP

#include "MenuBase.hpp"

#include <libtcod.h>

#include <memory>
#include <vector>

namespace tutorial
{
	class Engine;

	// Manages a stack of active menus
	// Handles rendering and input for the top menu
	class MenuStack
	{
	public:
		// Add a menu to the top of the stack
		void Push(std::unique_ptr<MenuBase> menu);

		// Remove the top menu from the stack
		void Pop();

		// Remove all menus from the stack
		void Clear();

		// Get the top menu (currently active)
		MenuBase* GetTop() const;

		// Check if the stack is empty
		bool IsEmpty() const;

		// Render the top menu with appropriate background
		void Render(TCOD_Console* console, Engine& engine);

		// Handle input for the top menu
		// Returns true if menu handled input and stays active
		bool HandleInput();

	private:
		std::vector<std::unique_ptr<MenuBase>> stack_;

		// Render background based on mode
		void RenderBackground(TCOD_Console* console, BackgroundMode mode,
		                      Engine& engine);
	};

} // namespace tutorial

#endif // MENU_STACK_HPP
