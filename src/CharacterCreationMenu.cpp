#include "CharacterCreationMenu.hpp"

#include "Engine.hpp"

#include <SDL3/SDL.h>

namespace tutorial
{
	CharacterCreationMenu::CharacterCreationMenu(Engine& engine,
	                                             const std::string& title,
	                                             pos_t position, int width,
	                                             int height)
	    : MenuBase(engine)
	{
		window_ = std::make_unique<MenuWindow>(width, height, position,
		                                       title, true);

		// Simple class selection
		window_->Clear();
		window_->AddItem(MenuAction::CharacterClass1, "Warrior");
		window_->AddItem(MenuAction::CharacterClass2, "Rogue");
		window_->AddItem(MenuAction::CharacterClass3, "Mage");
	}

	void CharacterCreationMenu::Render(TCOD_Console* console)
	{
		window_->Render(console);
	}

	bool CharacterCreationMenu::HandleInput()
	{
		SDL_Event sdlEvent;

		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_EVENT_QUIT) {
				engine_.Quit();
				return false;
			}

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode key = sdlEvent.key.key;

				// Arrow key navigation
				if (key == SDLK_UP || key == SDLK_KP_8) {
					window_->SelectPrevious();
					return true;
				}
				if (key == SDLK_DOWN || key == SDLK_KP_2) {
					window_->SelectNext();
					return true;
				}

				// Letter selection
				if (key >= SDLK_A && key <= SDLK_Z) {
					char letter =
					    static_cast<char>(key - SDLK_A + 'a');
					window_->SelectByLetter(letter);
					return true;
				}

				// Confirm selection - start new game with selected class
				if (key == SDLK_RETURN || key == SDLK_KP_ENTER
				    || key == SDLK_SPACE) {
					MenuAction action =
					    window_->GetSelectedAction();
					engine_.HandleMenuAction(action);
					return true;
				}

				// ESC key - return to start menu
				if (key == SDLK_ESCAPE) {
					engine_.ShowStartMenu();
					return false;
				}
			}
		}

		return true; // Menu stays active
	}

	BackgroundMode CharacterCreationMenu::GetBackgroundMode() const
	{
		return BackgroundMode::None;
	}

} // namespace tutorial
