#include "MenuStack.hpp"

#include "Engine.hpp"

namespace tutorial
{
	void MenuStack::Push(std::unique_ptr<MenuBase> menu)
	{
		if (menu) {
			stack_.push_back(std::move(menu));
		}
	}

	void MenuStack::Pop()
	{
		if (!stack_.empty()) {
			stack_.pop_back();
		}
	}

	void MenuStack::Clear()
	{
		stack_.clear();
	}

	MenuBase* MenuStack::GetTop() const
	{
		if (stack_.empty()) {
			return nullptr;
		}
		return stack_.back().get();
	}

	bool MenuStack::IsEmpty() const
	{
		return stack_.empty();
	}

	void MenuStack::Render(TCOD_Console* console, Engine& engine)
	{
		if (stack_.empty()) {
			return;
		}

		MenuBase* topMenu = GetTop();
		BackgroundMode bgMode = topMenu->GetBackgroundMode();

		// Render background first
		RenderBackground(console, bgMode, engine);

		// Render the menu on top
		topMenu->Render(console);
	}

	bool MenuStack::HandleInput()
	{
		if (stack_.empty()) {
			return false;
		}

		MenuBase* topMenu = GetTop();
		return topMenu->HandleInput();
	}

	void MenuStack::RenderBackground(TCOD_Console* console,
	                                 BackgroundMode mode, Engine& engine)
	{
		switch (mode) {
			case BackgroundMode::None:
				// Full-screen menu - just clear console
				TCOD_console_clear(console);
				break;

			case BackgroundMode::DimmedGameWorld:
				// Render game, then dim it
				engine.RenderGameBackground(console);
				// Dim the background by drawing a semi-transparent overlay
				for (int y = 0; y < engine.GetConfig().height; ++y) {
					for (int x = 0; x < engine.GetConfig().width;
					     ++x) {
						TCOD_ColorRGBA fg, bg;
						int ch = 0;
						TCOD_console_get_char_ex(console, x, y,
						                         &ch, &fg,
						                         &bg);
						// Darken foreground and background by 50%
						fg.r /= 2;
						fg.g /= 2;
						fg.b /= 2;
						bg.r /= 2;
						bg.g /= 2;
						bg.b /= 2;
						TCOD_console_put_rgb(console, x, y, ch,
						                     fg, bg);
					}
				}
				break;

			case BackgroundMode::GameWorld:
				// Render game world normally
				engine.RenderGameBackground(console);
				break;
		}
	}

} // namespace tutorial
