#ifndef UI_WINDOW_HPP
#define UI_WINDOW_HPP

#include "Position.hpp"

#include <libtcod/console.hpp>

#include <cstddef>
#include <memory>

namespace tutorial
{
	class UiWindow
	{
	public:
		virtual ~UiWindow() = default;

		virtual void Render(TCOD_Console* parent) const = 0;
	};

	class UiWindowBase : public UiWindow
	{
	public:
		UiWindowBase(std::size_t width, std::size_t height, pos_t pos);
		virtual ~UiWindowBase();
		virtual void Render(TCOD_Console* parent) const;

	protected:
		void DrawBorder(TCOD_Console* console,
		                const tcod::ColorRGB& frameColor) const;

		TCOD_Console* console_;
		pos_t pos_;
	};

} // namespace tutorial

#endif // UI_WINDOW_HPP
