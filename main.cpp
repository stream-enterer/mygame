#include "ConfigManager.hpp"
#include "Configuration.hpp"
#include "Engine.hpp"
#include "StringTable.hpp"

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
    while (engine.IsRunning())
    {
        engine.GetInput();
        engine.HandleEvents();
        engine.Render();
    }

    return 0;
}
