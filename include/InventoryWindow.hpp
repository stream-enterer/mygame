#ifndef INVENTORY_WINDOW_HPP
#define INVENTORY_WINDOW_HPP

#include "Position.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

#include <cstddef>

namespace tutorial
{
	class Entity;
	class Engine;

	class InventoryWindow : public UiWindowBase
	{
	public:
		InventoryWindow(std::size_t width, std::size_t height,
		                pos_t pos, const Entity& player);

		void Render(TCOD_Console* parent) const override;

		void SetTitle(const std::string& title)
		{
			title_ = title;
		}

	private:
		std::string title_;

	private:
		const Entity& player_;
	};
} // namespace tutorial

#endif // INVENTORY_WINDOW_HPP
