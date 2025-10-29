#ifndef SPELL_MENU_WINDOW_HPP
#define SPELL_MENU_WINDOW_HPP

#include "Position.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

#include <cstddef>

namespace tutorial
{
	class Entity;
	class Engine;

	class SpellMenuWindow : public UiWindowBase
	{
	public:
		SpellMenuWindow(std::size_t width, std::size_t height,
		                pos_t pos, const Entity& player);

		void Render(TCOD_Console* parent) const override;

	private:
		const Entity& player_;
	};
} // namespace tutorial

#endif // SPELL_MENU_WINDOW_HPP
