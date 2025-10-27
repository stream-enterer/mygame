#include "CharacterCreationWindow.hpp"

#include "ConfigManager.hpp"
#include "StringTable.hpp"

#include <algorithm>
#include <iostream>

namespace tutorial
{
	CharacterCreationWindow::CharacterCreationWindow(
	    std::size_t width, std::size_t height, pos_t pos)
	    : UiWindowBase(width, height, pos), currentTab_(CreationTab::Race),
	      raceMenuIndex_(0), selectedRaceIndex_(-1), classMenuIndex_(0),
	      selectedClassIndex_(-1), statsMenuIndex_(0), availablePoints_(10)
	{
		// Calculate central menu area (same position as before)
		int centerWidth = 50;
		int centerHeight = 25;
		menuWidth_ = centerWidth;
		menuHeight_ = centerHeight;
		menuPos_ = pos_t { static_cast<int>(width) / 2 - centerWidth / 2,
			           static_cast<int>(height) / 2
			               - centerHeight / 2 };
		menuStartY_ = menuPos_.y;

		LoadRaceOptions();
		LoadClassOptions();
		InitializeStats();
	}

	void CharacterCreationWindow::LoadRaceOptions()
	{
		auto& st = StringTable::Instance();

		// NOTE: race.en_US.json should be loaded by Engine after
		// en_US.json TODO: Multi-language support - load
		// race.<locale>.json based on current locale

		// Load race order from locale
		std::vector<std::string> raceOrder = { "human", "elf", "dwarf" };

		for (const auto& raceId : raceOrder) {
			std::string nameKey = "races." + raceId + ".name";
			std::string descKey = "races." + raceId + ".description";

			if (st.Has(nameKey) && st.Has(descKey)) {
				raceOptions_.push_back(
				    { raceId, st.GetString(nameKey),
				      st.GetString(descKey) });
			}
		}

		// Fall back if nothing loaded
		if (raceOptions_.empty()) {
			std::cerr
			    << "[CharacterCreation] WARNING: No race options "
			       "loaded from locale, using fallback"
			    << std::endl;
			raceOptions_.push_back({ "human", "Human",
			                         "Versatile and adaptable" });
			raceOptions_.push_back({ "elf", "Elf",
			                         "Graceful and perceptive" });
			raceOptions_.push_back({ "dwarf", "Dwarf",
			                         "Sturdy and resilient" });
		}
	}

	void CharacterCreationWindow::LoadClassOptions()
	{
		auto& st = StringTable::Instance();

		// NOTE: class.en_US.json should be loaded by Engine after
		// en_US.json TODO: Multi-language support - load
		// class.<locale>.json based on current locale

		// Load class order from locale
		std::vector<std::string> classOrder = { "warrior", "rogue",
			                                "mage" };

		for (const auto& classId : classOrder) {
			std::string nameKey = "classes." + classId + ".name";
			std::string descKey =
			    "classes." + classId + ".description";

			if (st.Has(nameKey) && st.Has(descKey)) {
				classOptions_.push_back(
				    { classId, st.GetString(nameKey),
				      st.GetString(descKey) });
			}
		}

		// Fall back if nothing loaded
		if (classOptions_.empty()) {
			std::cerr
			    << "[CharacterCreation] WARNING: No class options "
			       "loaded from locale, using fallback"
			    << std::endl;
			classOptions_.push_back(
			    { "warrior", "Warrior",
			      "Masters of melee combat and heavy armor" });
			classOptions_.push_back(
			    { "rogue", "Rogue",
			      "Swift assassins specializing in critical "
			      "strikes" });
			classOptions_.push_back(
			    { "mage", "Mage",
			      "Wielders of arcane magic and elemental power" });
		}
	}

	void CharacterCreationWindow::InitializeStats()
	{
		auto& st = StringTable::Instance();

		// NOTE: stats.en_US.json should be loaded by Engine after
		// en_US.json TODO: Multi-language support - load
		// stats.<locale>.json based on current locale

		// Initialize stats with base values
		std::vector<std::string> statOrder = { "strength", "dexterity",
			                               "intelligence" };

		for (const auto& statId : statOrder) {
			std::string nameKey = "stats." + statId + ".name";
			std::string descKey = "stats." + statId + ".description";

			std::string name = st.Has(nameKey)
			                       ? st.GetString(nameKey)
			                       : statId;
			std::string desc = st.Has(descKey)
			                       ? st.GetString(descKey)
			                       : "";

			stats_.push_back({ statId, name, desc, 10 }); // Base 10
		}

		// Fall back if nothing loaded
		if (stats_.empty()) {
			std::cerr
			    << "[CharacterCreation] WARNING: No stats loaded "
			       "from locale, using fallback"
			    << std::endl;
			stats_.push_back(
			    { "strength", "Strength",
			      "Physical power", 10 });
			stats_.push_back({ "dexterity", "Dexterity",
			                   "Agility and reflexes", 10 });
			stats_.push_back(
			    { "intelligence", "Intelligence",
			      "Magical aptitude", 10 });
		}
	}

	void CharacterCreationWindow::Render(TCOD_Console* parent) const
	{
		if (!console_) {
			return;
		}

		auto& cfg = ConfigManager::Instance();
		auto frameColor = cfg.GetUIFrameColor();

		// Clear console
		TCOD_console_clear(console_);

		// Draw frame at screen edges
		const int width = TCOD_console_get_width(console_);
		const int height = TCOD_console_get_height(console_);

		for (int x = 0; x < width; ++x) {
			TCOD_console_put_rgb(console_, x, 0, ' ', NULL,
			                     &frameColor, TCOD_BKGND_SET);

			TCOD_console_put_rgb(console_, x, height - 1, ' ', NULL,
			                     &frameColor, TCOD_BKGND_SET);
		}

		for (int y = 1; y < height - 1; ++y) {
			TCOD_console_put_rgb(console_, 0, y, ' ', NULL,
			                     &frameColor, TCOD_BKGND_SET);

			TCOD_console_put_rgb(console_, width - 1, y, ' ', NULL,
			                     &frameColor, TCOD_BKGND_SET);
		}

		// Render tabs
		RenderTabs(console_, width);

		// Render current tab content
		switch (currentTab_) {
			case CreationTab::Race:
				RenderRaceMenu(console_, width, height);
				break;
			case CreationTab::Class:
				RenderClassMenu(console_, width, height);
				break;
			case CreationTab::Stats:
				RenderStatsMenu(console_, width, height);
				break;
			case CreationTab::Confirm:
				RenderConfirmMenu(console_, width, height);
				break;
		}

		// Blit to parent
		TCOD_console_blit(console_, 0, 0, width, height, parent, pos_.x,
		                  pos_.y, 1.0f, 1.0f);
	}

	void CharacterCreationWindow::RenderTabs(TCOD_Console* console,
	                                         int width) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Tab names
		std::vector<std::string> tabNames = { "RACE", "CLASS", "STATS",
			                              "CONFIRM" };

		// Calculate tab positions (1/4 width each)
		int tabY = menuStartY_ - 2;
		int tabWidth = menuWidth_ / 4;
		int tabStartX = menuPos_.x;

		for (size_t i = 0; i < tabNames.size(); ++i) {
			const std::string& name = tabNames[i];
			bool isSelected =
			    (static_cast<int>(i)
			     == static_cast<int>(currentTab_));

			tcod::ColorRGB color =
			    isSelected ? highlightColor : textColor;

			// Calculate center position for this tab
			int tabX = tabStartX + static_cast<int>(i) * tabWidth;
			int textX = tabX + tabWidth / 2
			            - static_cast<int>(name.length()) / 2;

			// Draw tab text with color highlighting
			TCOD_printf_rgb(console,
			                (TCOD_PrintParamsRGB) {
			                    .x = textX,
			                    .y = tabY,
			                    .width = 0,
			                    .height = 0,
			                    .fg = &color,
			                    .bg = NULL,
			                    .flag = TCOD_BKGND_NONE,
			                    .alignment = TCOD_LEFT },
			                "%s", name.c_str());
		}
	}

	void CharacterCreationWindow::RenderRaceMenu(TCOD_Console* console,
	                                              int width,
	                                              int height) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Draw title
		std::string title = "Choose Your Race";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(console,
		                (TCOD_PrintParamsRGB) { .x = titleX,
		                                        .y = titleY,
		                                        .width = 0,
		                                        .height = 0,
		                                        .fg = &textColor,
		                                        .bg = NULL,
		                                        .flag = TCOD_BKGND_NONE,
		                                        .alignment = TCOD_LEFT },
		                "%s", title.c_str());

		// Draw race options
		int startY = menuStartY_ + 5;
		for (size_t i = 0; i < raceOptions_.size(); ++i) {
			bool isSelected = (static_cast<int>(i) == raceMenuIndex_);
			bool isMarked =
			    (static_cast<int>(i) == selectedRaceIndex_);

			tcod::ColorRGB color =
			    isSelected ? highlightColor : textColor;

			// Build item text with letter and marker
			std::string itemText;
			if (isMarked) {
				itemText += "* ";
			} else {
				itemText += "  ";
			}

			char letter = 'a' + static_cast<char>(i);
			itemText += "(";
			itemText += letter;
			itemText += ") ";
			itemText += raceOptions_[i].name + " - "
			            + raceOptions_[i].description;

			int itemY = startY + static_cast<int>(i) * 2;
			int itemX =
			    width / 2 - static_cast<int>(itemText.length()) / 2;

			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = itemX,
			                            .y = itemY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &color,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", itemText.c_str());
		}
	}

	void CharacterCreationWindow::RenderClassMenu(TCOD_Console* console,
	                                               int width,
	                                               int height) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Draw title
		std::string title = "Choose Your Class";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(console,
		                (TCOD_PrintParamsRGB) { .x = titleX,
		                                        .y = titleY,
		                                        .width = 0,
		                                        .height = 0,
		                                        .fg = &textColor,
		                                        .bg = NULL,
		                                        .flag = TCOD_BKGND_NONE,
		                                        .alignment = TCOD_LEFT },
		                "%s", title.c_str());

		// Draw class options
		int startY = menuStartY_ + 5;
		for (size_t i = 0; i < classOptions_.size(); ++i) {
			bool isSelected = (static_cast<int>(i) == classMenuIndex_);
			bool isMarked =
			    (static_cast<int>(i) == selectedClassIndex_);

			tcod::ColorRGB color =
			    isSelected ? highlightColor : textColor;

			// Build item text with letter and marker
			std::string itemText;
			if (isMarked) {
				itemText += "* ";
			} else {
				itemText += "  ";
			}

			char letter = 'a' + static_cast<char>(i);
			itemText += "(";
			itemText += letter;
			itemText += ") ";
			itemText += classOptions_[i].name + " - "
			            + classOptions_[i].description;

			int itemY = startY + static_cast<int>(i) * 2;
			int itemX =
			    width / 2 - static_cast<int>(itemText.length()) / 2;

			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = itemX,
			                            .y = itemY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &color,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", itemText.c_str());
		}
	}

	void CharacterCreationWindow::RenderStatsMenu(TCOD_Console* console,
	                                               int width,
	                                               int height) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Draw title
		std::string title = "Allocate Stat Points";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(console,
		                (TCOD_PrintParamsRGB) { .x = titleX,
		                                        .y = titleY,
		                                        .width = 0,
		                                        .height = 0,
		                                        .fg = &textColor,
		                                        .bg = NULL,
		                                        .flag = TCOD_BKGND_NONE,
		                                        .alignment = TCOD_LEFT },
		                "%s", title.c_str());

		// Show available points
		std::string pointsText =
		    "Available Points: " + std::to_string(availablePoints_);
		int pointsX =
		    width / 2 - static_cast<int>(pointsText.length()) / 2;
		TCOD_printf_rgb(console,
		                (TCOD_PrintParamsRGB) { .x = pointsX,
		                                        .y = titleY + 1,
		                                        .width = 0,
		                                        .height = 0,
		                                        .fg = &textColor,
		                                        .bg = NULL,
		                                        .flag = TCOD_BKGND_NONE,
		                                        .alignment = TCOD_LEFT },
		                "%s", pointsText.c_str());

		// Draw stats
		int startY = menuStartY_ + 6;
		for (size_t i = 0; i < stats_.size(); ++i) {
			bool isSelected = (static_cast<int>(i) == statsMenuIndex_);

			tcod::ColorRGB color =
			    isSelected ? highlightColor : textColor;

			std::string itemText =
			    stats_[i].name + ": "
			    + std::to_string(stats_[i].value);

			int itemY = startY + static_cast<int>(i) * 2;
			int itemX =
			    width / 2 - static_cast<int>(itemText.length()) / 2;

			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = itemX,
			                            .y = itemY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &color,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", itemText.c_str());
		}

		// Instructions
		std::string instructions = "Use +/- keys to adjust stats";
		int instrX =
		    width / 2 - static_cast<int>(instructions.length()) / 2;
		TCOD_printf_rgb(
		    console,
		    (TCOD_PrintParamsRGB) { .x = instrX,
		                            .y = startY
		                                + static_cast<int>(
		                                      stats_.size() * 2)
		                                + 2,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &textColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", instructions.c_str());
	}

	void CharacterCreationWindow::RenderConfirmMenu(TCOD_Console* console,
	                                                 int width,
	                                                 int height) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();

		// Draw title
		std::string title = "Confirm Your Character";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(console,
		                (TCOD_PrintParamsRGB) { .x = titleX,
		                                        .y = titleY,
		                                        .width = 0,
		                                        .height = 0,
		                                        .fg = &textColor,
		                                        .bg = NULL,
		                                        .flag = TCOD_BKGND_NONE,
		                                        .alignment = TCOD_LEFT },
		                "%s", title.c_str());

		// Show selected options
		int startY = menuStartY_ + 5;
		int currentY = startY;

		// Race
		if (selectedRaceIndex_ >= 0
		    && selectedRaceIndex_
		           < static_cast<int>(raceOptions_.size())) {
			std::string raceText =
			    "Race: " + raceOptions_[selectedRaceIndex_].name;
			int raceX =
			    width / 2 - static_cast<int>(raceText.length()) / 2;
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = raceX,
			                            .y = currentY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", raceText.c_str());
			currentY += 2;
		}

		// Class
		if (selectedClassIndex_ >= 0
		    && selectedClassIndex_
		           < static_cast<int>(classOptions_.size())) {
			std::string classText =
			    "Class: " + classOptions_[selectedClassIndex_].name;
			int classX =
			    width / 2 - static_cast<int>(classText.length()) / 2;
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = classX,
			                            .y = currentY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", classText.c_str());
			currentY += 2;
		}

		// Stats
		for (const auto& stat : stats_) {
			std::string statText =
			    stat.name + ": " + std::to_string(stat.value);
			int statX =
			    width / 2 - static_cast<int>(statText.length()) / 2;
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = statX,
			                            .y = currentY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", statText.c_str());
			currentY += 1;
		}

		// Instructions
		currentY += 2;
		std::string instructions = "Press Enter to begin your adventure!";
		int instrX =
		    width / 2 - static_cast<int>(instructions.length()) / 2;
		TCOD_printf_rgb(console,
		                (TCOD_PrintParamsRGB) { .x = instrX,
		                                        .y = currentY,
		                                        .width = 0,
		                                        .height = 0,
		                                        .fg = &textColor,
		                                        .bg = NULL,
		                                        .flag = TCOD_BKGND_NONE,
		                                        .alignment = TCOD_LEFT },
		                "%s", instructions.c_str());
	}

	void CharacterCreationWindow::SelectNextTab()
	{
		int tabIndex = static_cast<int>(currentTab_);
		tabIndex = (tabIndex + 1) % 4;
		currentTab_ = static_cast<CreationTab>(tabIndex);
	}

	void CharacterCreationWindow::SelectPreviousTab()
	{
		int tabIndex = static_cast<int>(currentTab_);
		tabIndex = (tabIndex - 1 + 4) % 4;
		currentTab_ = static_cast<CreationTab>(tabIndex);
	}

	void CharacterCreationWindow::SelectTab(CreationTab tab)
	{
		currentTab_ = tab;
	}

	void CharacterCreationWindow::SelectPrevious()
	{
		switch (currentTab_) {
			case CreationTab::Race:
				if (!raceOptions_.empty()) {
					raceMenuIndex_ =
					    (raceMenuIndex_ - 1
					     + static_cast<int>(
					         raceOptions_.size()))
					    % static_cast<int>(
					        raceOptions_.size());
				}
				break;

			case CreationTab::Class:
				if (!classOptions_.empty()) {
					classMenuIndex_ =
					    (classMenuIndex_ - 1
					     + static_cast<int>(
					         classOptions_.size()))
					    % static_cast<int>(
					        classOptions_.size());
				}
				break;

			case CreationTab::Stats:
				if (!stats_.empty()) {
					statsMenuIndex_ =
					    (statsMenuIndex_ - 1
					     + static_cast<int>(stats_.size()))
					    % static_cast<int>(stats_.size());
				}
				break;

			case CreationTab::Confirm:
				// No navigation in confirm tab
				break;
		}
	}

	void CharacterCreationWindow::SelectNext()
	{
		switch (currentTab_) {
			case CreationTab::Race:
				if (!raceOptions_.empty()) {
					raceMenuIndex_ =
					    (raceMenuIndex_ + 1)
					    % static_cast<int>(
					        raceOptions_.size());
				}
				break;

			case CreationTab::Class:
				if (!classOptions_.empty()) {
					classMenuIndex_ =
					    (classMenuIndex_ + 1)
					    % static_cast<int>(
					        classOptions_.size());
				}
				break;

			case CreationTab::Stats:
				if (!stats_.empty()) {
					statsMenuIndex_ =
					    (statsMenuIndex_ + 1)
					    % static_cast<int>(stats_.size());
				}
				break;

			case CreationTab::Confirm:
				// No navigation in confirm tab
				break;
		}
	}

	bool CharacterCreationWindow::SelectByLetter(char letter)
	{
		// Convert to lowercase
		if (letter >= 'A' && letter <= 'Z') {
			letter = letter - 'A' + 'a';
		}

		int index = letter - 'a';

		switch (currentTab_) {
			case CreationTab::Race:
				if (index >= 0
				    && index
				           < static_cast<int>(
				               raceOptions_.size())) {
					raceMenuIndex_ = index;
					selectedRaceIndex_ = index; // Mark it
					return true;
				}
				break;

			case CreationTab::Class:
				if (index >= 0
				    && index
				           < static_cast<int>(
				               classOptions_.size())) {
					classMenuIndex_ = index;
					selectedClassIndex_ = index; // Mark it
					return true;
				}
				break;

			case CreationTab::Stats:
			case CreationTab::Confirm:
				// No letter selection in these tabs
				break;
		}

		return false;
	}

	void CharacterCreationWindow::ConfirmSelection()
	{
		switch (currentTab_) {
			case CreationTab::Race:
				// If already marked, advance to next tab
				if (selectedRaceIndex_ == raceMenuIndex_) {
					SelectNextTab();
				} else {
					// Mark the selection
					selectedRaceIndex_ = raceMenuIndex_;
				}
				break;

			case CreationTab::Class:
				// If already marked, advance to next tab
				if (selectedClassIndex_ == classMenuIndex_) {
					SelectNextTab();
				} else {
					// Mark the selection
					selectedClassIndex_ = classMenuIndex_;
				}
				break;

			case CreationTab::Stats:
				// Stats don't get "confirmed" - just advance
				SelectNextTab();
				break;

			case CreationTab::Confirm:
				// Handled by Engine
				break;
		}
	}

	void CharacterCreationWindow::IncrementStat()
	{
		if (currentTab_ != CreationTab::Stats || stats_.empty()) {
			return;
		}

		if (availablePoints_ > 0) {
			stats_[statsMenuIndex_].value++;
			availablePoints_--;
		}
	}

	void CharacterCreationWindow::DecrementStat()
	{
		if (currentTab_ != CreationTab::Stats || stats_.empty()) {
			return;
		}

		if (stats_[statsMenuIndex_].value > 1) {
			stats_[statsMenuIndex_].value--;
			availablePoints_++;
		}
	}

	bool CharacterCreationWindow::IsReadyToConfirm() const
	{
		return selectedRaceIndex_ >= 0 && selectedClassIndex_ >= 0;
	}

} // namespace tutorial
