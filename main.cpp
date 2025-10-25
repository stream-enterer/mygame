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

	static const tutorial::Configuration config {
		"libtcod C++ tutorial 8", // title
		80,                       // width
		50,                       // height
		60,                       // fps
		"font.bdf"                // fontPath
	};
	tutorial::Engine engine { config };
	tutorial::TurnManager turnManager;

	while (engine.IsRunning()) {
		auto command = engine.GetInput();
		turnManager.ProcessCommand(std::move(command), engine);
		engine.Render();
	}

	return 0;
}
