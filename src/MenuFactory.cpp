#include "MenuFactory.hpp"

#include "CharacterCreationMenu.hpp"
#include "Engine.hpp"
#include "ListMenu.hpp"
#include "SaveManager.hpp"

namespace tutorial
{
	std::unique_ptr<MenuBase> MenuFactory::CreatePauseMenu(Engine& engine)
	{
		int width = 40;
		int height = 20;
		pos_t pos = CalculateCenteredPosition(width, height, engine);

		auto menu = std::make_unique<ListMenu>(
		    engine, "PAUSED", pos, width, height,
		    BackgroundMode::DimmedGameWorld);

		menu->AddItem(MenuAction::Continue, "Resume Game");
		menu->AddItem(MenuAction::SaveAndQuit, "Save and Quit");

		return menu;
	}

	std::unique_ptr<MenuBase> MenuFactory::CreateStartMenu(Engine& engine)
	{
		int width = static_cast<int>(engine.GetConfig().width);
		int height = static_cast<int>(engine.GetConfig().height);
		pos_t pos { 0, 0 };

		auto menu = std::make_unique<ListMenu>(
		    engine, "My Game", pos, width, height,
		    BackgroundMode::None, true); // fullScreenBorder = true

		menu->SetGameLogoStub("[GameLogo]");
		menu->Clear();

		// Show "Continue" if a save file exists
		if (SaveManager::Instance().HasSave()) {
			menu->AddItem(MenuAction::Continue, "Continue");
		}

		menu->AddItem(MenuAction::NewGame, "New Game");
		menu->AddItem(MenuAction::Quit, "Exit");

		return menu;
	}

	std::unique_ptr<MenuBase>
	MenuFactory::CreateLevelUpMenu(Engine& engine)
	{
		int width = 50;
		int height = 18;
		pos_t pos = CalculateCenteredPosition(width, height, engine);

		auto menu = std::make_unique<ListMenu>(
		    engine, "Level Up!", pos, width, height,
		    BackgroundMode::DimmedGameWorld);

		menu->AddItem(MenuAction::LevelUpStrength, "Strength (+1 attack)");
		menu->AddItem(MenuAction::LevelUpDexterity,
		              "Dexterity (+1 defense)");
		menu->AddItem(MenuAction::LevelUpIntelligence,
		              "Intelligence (+1 mana)");

		return menu;
	}

	std::unique_ptr<MenuBase>
	MenuFactory::CreateNewGameConfirmation(Engine& engine)
	{
		int width = 50;
		int height = 15;
		pos_t pos = CalculateCenteredPosition(width, height, engine);

		auto menu = std::make_unique<ListMenu>(
		    engine, "Abandon Current Save?", pos, width, height,
		    BackgroundMode::None);

		menu->AddItem(MenuAction::ConfirmNo, "No - Return to Menu");
		menu->AddItem(MenuAction::ConfirmYes, "Yes - Start New Game");

		return menu;
	}

	std::unique_ptr<MenuBase>
	MenuFactory::CreateCharacterCreation(Engine& engine)
	{
		int width = static_cast<int>(engine.GetConfig().width);
		int height = static_cast<int>(engine.GetConfig().height);
		pos_t pos { 0, 0 };

		return std::make_unique<CharacterCreationMenu>(
		    engine, "Choose Your Class", pos, width, height);
	}

	pos_t MenuFactory::CalculateCenteredPosition(int width, int height,
	                                             const Engine& engine)
	{
		return pos_t { static_cast<int>(engine.GetConfig().width) / 2
			           - width / 2,
			       static_cast<int>(engine.GetConfig().height) / 2
			           - height / 2 };
	}

} // namespace tutorial
