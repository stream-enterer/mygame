#include "UiWindow.hpp"

#include <libtcod/console.hpp>
#include <memory>

namespace tutorial
{
	UiWindowBase::UiWindowBase(std::size_t width, std::size_t height,
	                           pos_t pos)
	    : console_(TCOD_console_new(width, height)), pos_(pos)
	{
	}

	UiWindowBase::~UiWindowBase()
	{
		if (console_) {
			TCOD_console_delete(console_);
		}
	}

	void UiWindowBase::Render(TCOD_Console*) const
	{
		// No op
	}

	void UiWindowBase::DrawBorder(TCOD_Console* console,
	                              const tcod::ColorRGB& frameColor) const
	{
		const int width = TCOD_console_get_width(console);
		const int height = TCOD_console_get_height(console);

		// Draw corners
		TCOD_console_put_rgb(console, 0, 0, 0x2219, &frameColor, NULL,
		                     TCOD_BKGND_SET); // Top-left: ∙
		TCOD_console_put_rgb(console, width - 1, 0, 0x2219, &frameColor,
		                     NULL,
		                     TCOD_BKGND_SET); // Top-right: ∙
		TCOD_console_put_rgb(console, 0, height - 1, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Bottom-left: ∙
		TCOD_console_put_rgb(console, width - 1, height - 1, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Bottom-right: ∙

		// Draw horizontal edges
		for (int x = 1; x < width - 1; ++x) {
			TCOD_console_put_rgb(console, x, 0, 0x2550, &frameColor,
			                     NULL,
			                     TCOD_BKGND_SET); // Top: ═
			TCOD_console_put_rgb(console, x, height - 1, 0x2550,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Bottom: ═
		}

		// Draw vertical edges
		for (int y = 1; y < height - 1; ++y) {
			TCOD_console_put_rgb(console, 0, y, 0x2551, &frameColor,
			                     NULL,
			                     TCOD_BKGND_SET); // Left: ║
			TCOD_console_put_rgb(console, width - 1, y, 0x2551,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Right: ║
		}
	}
} // namespace tutorial
