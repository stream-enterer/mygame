#ifndef ITEM_SELECTION_WINDOW_HPP
#define ITEM_SELECTION_WINDOW_HPP

#include "Position.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace tutorial
{
	class Entity;

	// Generic window for selecting items from a list
	class ItemSelectionWindow : public UiWindowBase
	{
	public:
		ItemSelectionWindow(std::size_t width, std::size_t height,
		                    pos_t pos,
		                    const std::vector<Entity*>& items,
		                    const std::string& title);

		void Render(TCOD_Console* parent) const override;

	private:
		const std::vector<Entity*>& items_;
		std::string title_;
	};
} // namespace tutorial

#endif // ITEM_SELECTION_WINDOW_HPP
