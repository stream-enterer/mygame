#include "CharacterCreationWindow.hpp"

#include "ConfigManager.hpp"
#include "LocaleManager.hpp"

#include <algorithm>
#include <iostream>

namespace tutorial
{
	CharacterCreationWindow::CharacterCreationWindow(std::size_t width,
	                                                 std::size_t height,
	                                                 pos_t pos)
	    : UiWindowBase(width, height, pos),
	      currentTab_(CreationTab::Species),
	      speciesMenuIndex_(0),
	      selectedSpeciesIndex_(-1),
	      classMenuIndex_(0),
	      selectedClassIndex_(-1),
	      statsMenuIndex_(0),
	      availablePoints_(10)
	{
		// Calculate central menu area (same position as before)
		int centerWidth = 50;
		int centerHeight = 25;
		menuWidth_ = centerWidth;
		menuHeight_ = centerHeight;
		menuPos_ =
		    pos_t { static_cast<int>(width) / 2 - centerWidth / 2,
			    static_cast<int>(height) / 2 - centerHeight / 2 };
		menuStartY_ = menuPos_.y;

		LoadSpeciesOptions();
		LoadClassOptions();
		InitializeStats();
	}

	void CharacterCreationWindow::LoadSpeciesOptions()
	{
		auto& lm = LocaleManager::Instance();
		const auto& speciesData = lm.GetSpecies();

		speciesOptions_.clear();
		for (const auto& species : speciesData) {
			speciesOptions_.push_back({ species.name, species.name,
			                            species.description });
		}

		std::cout << "[CharacterCreation] Loaded "
		          << speciesOptions_.size()
		          << " species from LocaleManager" << std::endl;
	}

	void CharacterCreationWindow::LoadClassOptions()
	{
		auto& lm = LocaleManager::Instance();
		const auto& classData = lm.GetClasses();

		classOptions_.clear();
		for (const auto& cls : classData) {
			classOptions_.push_back(
			    { cls.name, cls.name, cls.description });
		}

		std::cout << "[CharacterCreation] Loaded "
		          << classOptions_.size()
		          << " classes from LocaleManager" << std::endl;
	}

	void CharacterCreationWindow::InitializeStats()
	{
		auto& st = LocaleManager::Instance();

		std::vector<std::string> statOrder = { "strength", "dexterity",
			                               "intelligence" };

		for (const auto& statId : statOrder) {
			std::string nameKey = "stats." + statId + ".name";
			std::string descKey =
			    "stats." + statId + ".description";

			std::string name =
			    st.Has(nameKey) ? st.GetString(nameKey) : statId;
			std::string desc =
			    st.Has(descKey) ? st.GetString(descKey) : "";

			stats_.push_back({ statId, name, desc, 10 }); // Base 10
		}

		// Fall back if nothing loaded
		if (stats_.empty()) {
			std::cerr
			    << "[CharacterCreation] WARNING: No stats loaded "
			       "from locale, using fallback"
			    << std::endl;
			stats_.push_back(
			    { "strength", "Strength", "Physical power", 10 });
			stats_.push_back({ "dexterity", "Dexterity",
			                   "Agility and reflexes", 10 });
			stats_.push_back({ "intelligence", "Intelligence",
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

		// Draw corners
		TCOD_console_put_rgb(console_, 0, 0, 0x2219, &frameColor, NULL,
		                     TCOD_BKGND_SET); // Top-left: ∙
		TCOD_console_put_rgb(console_, width - 1, 0, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Top-right: ∙
		TCOD_console_put_rgb(console_, 0, height - 1, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Bottom-left: ∙
		TCOD_console_put_rgb(console_, width - 1, height - 1, 0x2219,
		                     &frameColor, NULL,
		                     TCOD_BKGND_SET); // Bottom-right: ∙

		// Draw horizontal edges
		for (int x = 1; x < width - 1; ++x) {
			TCOD_console_put_rgb(console_, x, 0, 0x2550,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Top: ═
			TCOD_console_put_rgb(console_, x, height - 1, 0x2550,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Bottom: ═
		}

		// Draw vertical edges
		for (int y = 1; y < height - 1; ++y) {
			TCOD_console_put_rgb(console_, 0, y, 0x2551,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Left: ║
			TCOD_console_put_rgb(console_, width - 1, y, 0x2551,
			                     &frameColor, NULL,
			                     TCOD_BKGND_SET); // Right: ║
		}

		// Render tabs
		RenderTabs(console_, width);

		// Render current tab content
		switch (currentTab_) {
			case CreationTab::Species:
				RenderSpeciesMenu(console_, width, height);
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
	                                         int /*width*/) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Tab names
		std::vector<std::string> tabNames = { "SPECIES", "CLASS",
			                              "STATS", "CONFIRM" };

		// Calculate tab positions (1/4 width each)
		int tabY = menuStartY_ - 2;
		int tabWidth = menuWidth_ / 4;
		int tabStartX = menuPos_.x;

		for (size_t i = 0; i < tabNames.size(); ++i) {
			const std::string& name = tabNames[i];
			bool isSelected = (static_cast<int>(i)
			                   == static_cast<int>(currentTab_));

			tcod::ColorRGB color =
			    isSelected ? highlightColor : textColor;

			// Calculate center position for this tab
			int tabX = tabStartX + static_cast<int>(i) * tabWidth;
			int textX = tabX + tabWidth / 2
			            - static_cast<int>(name.length()) / 2;

			// Draw tab text with color highlighting
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = textX,
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

	void CharacterCreationWindow::RenderSpeciesMenu(TCOD_Console* console,
	                                                int width,
	                                                int /*height*/) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();

		// Draw title
		std::string title = "Choose Your Species";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(
		    console,
		    (TCOD_PrintParamsRGB) { .x = titleX,
		                            .y = titleY,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &textColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", title.c_str());

		// Calculate bounds based on tab labels
		// Left: Start of 'S' in "SPECIES" (position 3 from tab
		// rendering) Right: Position of 'M' in "CONFIRM"
		int tabStartX = menuPos_.x;
		int leftBound = tabStartX;
		int rightBound = tabStartX + (menuWidth_ / 4) * 3
		                 + 6; // Approximately to 'M' in CONFIRM

		// Render three-column species list
		int currentY = menuStartY_ + 5;
		RenderThreeColumnList(console, speciesOptions_,
		                      speciesMenuIndex_, leftBound, rightBound,
		                      currentY);

		// Add line break
		currentY += 2;

		// Render description for currently highlighted species
		if (speciesMenuIndex_ >= 0
		    && speciesMenuIndex_
		           < static_cast<int>(speciesOptions_.size())) {
			const std::string& description =
			    speciesOptions_[speciesMenuIndex_].description;
			RenderDescriptionBlock(console, description, leftBound,
			                       rightBound, currentY);
		}
	}

	void CharacterCreationWindow::RenderClassMenu(TCOD_Console* console,
	                                              int width,
	                                              int /*height*/) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();

		// Draw title
		std::string title = "Choose Your Class";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(
		    console,
		    (TCOD_PrintParamsRGB) { .x = titleX,
		                            .y = titleY,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &textColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", title.c_str());

		// Calculate bounds (same as species menu)
		int tabStartX = menuPos_.x;
		int leftBound = tabStartX;
		int rightBound = tabStartX + (menuWidth_ / 4) * 3 + 6;

		// Render three-column class list
		int currentY = menuStartY_ + 5;
		RenderThreeColumnList(console, classOptions_, classMenuIndex_,
		                      leftBound, rightBound, currentY);

		// Add line break
		currentY += 2;

		// Render description for currently highlighted class
		if (classMenuIndex_ >= 0
		    && classMenuIndex_
		           < static_cast<int>(classOptions_.size())) {
			const std::string& description =
			    classOptions_[classMenuIndex_].description;
			RenderDescriptionBlock(console, description, leftBound,
			                       rightBound, currentY);
		}
	}

	void CharacterCreationWindow::RenderStatsMenu(TCOD_Console* console,
	                                              int width,
	                                              int /*height*/) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Draw title
		std::string title = "Allocate Stat Points";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(
		    console,
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
		TCOD_printf_rgb(
		    console,
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
			bool isSelected =
			    (static_cast<int>(i) == statsMenuIndex_);

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
		    (TCOD_PrintParamsRGB) {
		        .x = instrX,
		        .y = startY + static_cast<int>(stats_.size() * 2) + 2,
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
	                                                int /*height*/) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();

		// Draw title
		std::string title = "Confirm Your Character";
		int titleX = width / 2 - static_cast<int>(title.length()) / 2;
		int titleY = menuStartY_ + 2;

		TCOD_printf_rgb(
		    console,
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

		// Species
		if (selectedSpeciesIndex_ >= 0
		    && selectedSpeciesIndex_
		           < static_cast<int>(speciesOptions_.size())) {
			std::string speciesText =
			    "Species: "
			    + speciesOptions_[selectedSpeciesIndex_].name;
			int speciesX =
			    width / 2
			    - static_cast<int>(speciesText.length()) / 2;
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = speciesX,
			                            .y = currentY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", speciesText.c_str());
			currentY += 2;
		}

		// Class
		if (selectedClassIndex_ >= 0
		    && selectedClassIndex_
		           < static_cast<int>(classOptions_.size())) {
			std::string classText =
			    "Class: " + classOptions_[selectedClassIndex_].name;
			int classX = width / 2
			             - static_cast<int>(classText.length()) / 2;
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
		std::string instructions =
		    "Press Enter to begin your adventure!";
		int instrX =
		    width / 2 - static_cast<int>(instructions.length()) / 2;
		TCOD_printf_rgb(
		    console,
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
			case CreationTab::Species:
				if (!speciesOptions_.empty()) {
					speciesMenuIndex_ =
					    (speciesMenuIndex_ - 1
					     + static_cast<int>(
					         speciesOptions_.size()))
					    % static_cast<int>(
					        speciesOptions_.size());
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
			case CreationTab::Species:
				if (!speciesOptions_.empty()) {
					speciesMenuIndex_ =
					    (speciesMenuIndex_ + 1)
					    % static_cast<int>(
					        speciesOptions_.size());
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
			case CreationTab::Species:
				if (index >= 0
				    && index < static_cast<int>(
				           speciesOptions_.size())) {
					speciesMenuIndex_ = index;
					selectedSpeciesIndex_ =
					    index; // Mark it
					return true;
				}
				break;

			case CreationTab::Class:
				if (index >= 0
				    && index < static_cast<int>(
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
			case CreationTab::Species:
				// If already marked, advance to next tab
				if (selectedSpeciesIndex_
				    == speciesMenuIndex_) {
					SelectNextTab();
				} else {
					// Mark the selection
					selectedSpeciesIndex_ =
					    speciesMenuIndex_;
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

	void CharacterCreationWindow::RenderThreeColumnList(
	    TCOD_Console* console, const std::vector<CreationOption>& items,
	    int highlightIndex, int leftBound, int rightBound,
	    int& currentY) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		const int totalWidth = rightBound - leftBound;
		const int columnWidth = totalWidth / 3;
		const int itemsPerColumn = 4;

		int maxY = currentY;

		for (int col = 0; col < 3; ++col) {
			int colX = leftBound + (col * columnWidth);
			int colY = currentY;

			for (int row = 0; row < itemsPerColumn; ++row) {
				int itemIndex = (col * itemsPerColumn) + row;
				if (itemIndex
				    >= static_cast<int>(items.size())) {
					break;
				}

				// Convert index to letter (0='a', 1='b', etc.)
				char letter = 'a' + itemIndex;

				// Truncate name if it exceeds column width
				// Reserve 4 chars for "(x) " prefix
				int maxNameWidth = columnWidth - 5;
				std::string displayName = items[itemIndex].name;
				if (static_cast<int>(displayName.length())
				    > maxNameWidth) {
					// HARD CUT: Truncate name without
					// ellipsis
					displayName =
					    displayName.substr(0, maxNameWidth);
				}

				std::string line = "(" + std::string(1, letter)
				                   + ") " + displayName;

				// Highlight if this is the selected item
				tcod::ColorRGB color =
				    (itemIndex == highlightIndex)
				        ? highlightColor
				        : textColor;

				TCOD_printf_rgb(console,
				                (TCOD_PrintParamsRGB) {
				                    .x = colX,
				                    .y = colY,
				                    .width = 0,
				                    .height = 0,
				                    .fg = &color,
				                    .bg = NULL,
				                    .flag = TCOD_BKGND_NONE,
				                    .alignment = TCOD_LEFT },
				                "%s", line.c_str());

				colY++;
				maxY = std::max(maxY, colY);
			}
		}

		currentY = maxY;
	}

	std::string CharacterCreationWindow::WrapText(const std::string& text,
	                                              int maxWidth) const
	{
		if (maxWidth <= 0) return text;

		std::string result;
		std::string currentLine;
		std::istringstream words(text);
		std::string word;

		while (words >> word) {
			// Check if adding this word would exceed width
			std::string testLine = currentLine.empty()
			                           ? word
			                           : currentLine + " " + word;

			if (static_cast<int>(testLine.length()) <= maxWidth) {
				currentLine = testLine;
			} else {
				// Flush current line and start new one
				if (!currentLine.empty()) {
					result += currentLine + "\n";
				}
				currentLine = word;
			}
		}

		// Add remaining text
		if (!currentLine.empty()) {
			result += currentLine;
		}

		return result;
	}

	void CharacterCreationWindow::RenderDescriptionBlock(
	    TCOD_Console* console, const std::string& description,
	    int leftBound, int rightBound, int startY) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();

		int maxWidth = rightBound - leftBound;
		std::string wrapped = WrapText(description, maxWidth);

		std::istringstream lines(wrapped);
		std::string line;
		int y = startY;

		while (std::getline(lines, line)) {
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = leftBound,
			                            .y = y,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", line.c_str());
			y++;
		}
	}

	bool CharacterCreationWindow::IsReadyToConfirm() const
	{
		return selectedSpeciesIndex_ >= 0 && selectedClassIndex_ >= 0;
	}

} // namespace tutorial
