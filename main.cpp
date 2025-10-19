#include "Command.hpp"
#include "ConfigManager.hpp"
#include "Configuration.hpp"
#include "Engine.hpp"
#include "SaveManager.hpp"
#include "StringTable.hpp"
#include "TurnManager.hpp"

#include <iostream>
#include <string>

int main()
{
    // Load all configuration files before creating engine
    tutorial::ConfigManager::Instance().LoadAll();

    // Load default locale
    tutorial::StringTable::Instance().LoadLocale("en_US");

    static const tutorial::Configuration config{ "libtcod C++ tutorial 8", 80,
                                                 50, 60 };
    tutorial::Engine engine{ config };
    tutorial::TurnManager turnManager;

    // Auto-load save if it exists (traditional roguelike behavior)
    if (tutorial::SaveManager::Instance().HasSave())
    {
        auto metadata = tutorial::SaveManager::Instance().GetSaveMetadata();

        std::cout << "==================================" << std::endl;
        std::cout << "Save file found!" << std::endl;
        std::cout << "Player: " << metadata.playerName << std::endl;
        std::cout << "HP: " << metadata.playerHP << "/" << metadata.playerMaxHP
                  << std::endl;
        std::cout << "Level: " << metadata.levelName << std::endl;
        std::cout << "Saved: " << metadata.timestamp << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Loading save..." << std::endl;

        if (!tutorial::SaveManager::Instance().LoadGame(engine))
        {
            std::cout << "Load failed, starting new game instead" << std::endl;
        }
    }

    while (engine.IsRunning())
    {
        auto command = engine.GetInput();
        turnManager.ProcessCommand(std::move(command), engine);
        engine.Render();
    }

    return 0;
}
