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
	      speciesOptions_(),
	      speciesGrid_(
	          ConfigManager::Instance().GetCharacterCreationGridColumns(),
	          ConfigManager::Instance()
	              .GetCharacterCreationGridItemsPerColumn(),
	          0),
	      selectedSpeciesIndex_(-1),
	      classOptions_(),
	      classGrid_(
	          ConfigManager::Instance().GetCharacterCreationGridColumns(),
	          ConfigManager::Instance()
	              .GetCharacterCreationGridItemsPerColumn(),
	          0),
	      selectedClassIndex_(-1),
	      stats_(),
	      statsMenuIndex_(0),
	      availablePoints_(10)
	{
		// Load menu dimensions from config
		auto& cfg = ConfigManager::Instance();
		menuWidth_ = cfg.GetCharacterCreationMenuWidth();
		menuHeight_ = cfg.GetCharacterCreationMenuHeight();

		// Center the menu on screen
		menuPos_ =
		    pos_t { static_cast<int>(width) / 2 - menuWidth_ / 2,
			    static_cast<int>(height) / 2 - menuHeight_ / 2 };
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

		// Update grid with actual item count
		speciesGrid_ = GridNavigator(
		    ConfigManager::Instance().GetCharacterCreationGridColumns(),
		    ConfigManager::Instance()
		        .GetCharacterCreationGridItemsPerColumn(),
		    static_cast<int>(speciesOptions_.size()));

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

		// Update grid with actual item count
		classGrid_ = GridNavigator(
		    ConfigManager::Instance().GetCharacterCreationGridColumns(),
		    ConfigManager::Instance()
		        .GetCharacterCreationGridItemsPerColumn(),
		    static_cast<int>(classOptions_.size()));

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

			stats_.push_back({ statId, name, desc, 8 });
		}
	}

	void CharacterCreationWindow::Render(TCOD_Console* parent) const
	{
		if (!console_) {
			return;
		}

		// Clear console
		TCOD_console_clear(console_);

		// Draw border
		auto& cfg = ConfigManager::Instance();
		auto frameColor = cfg.GetUIFrameColor();
		DrawBorder(console_, frameColor);

		// Get console dimensions for rendering
		const int width = TCOD_console_get_width(console_);
		const int height = TCOD_console_get_height(console_);

		// Render tabs at top
		RenderTabs(console_, width);

		// Render content based on current tab
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

		// Calculate tab positions (evenly distributed)
		int tabWidth = menuWidth_ / 4;
		int tabStartX = menuPos_.x;
		int tabY = menuStartY_;

		for (std::size_t i = 0; i < tabNames.size(); ++i) {
			const std::string& name = tabNames[i];
			bool isActive = (static_cast<int>(i)
			                 == static_cast<int>(currentTab_));

			// Choose color based on whether this is the active tab
			tcod::ColorRGB color =
			    isActive ? highlightColor : textColor;

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
	                                                int /*width*/,
	                                                int /*height*/) const
	{
		// Content bounds: full menu width
		int leftBound = menuPos_.x;
		int rightBound = menuPos_.x + menuWidth_;

		// Render three-column species list
		int currentY = menuStartY_ + 2;
		RenderThreeColumnList(
		    console, speciesOptions_, speciesGrid_.GetIndex(),
		    selectedSpeciesIndex_, leftBound, rightBound, currentY);

		// Add line break
		currentY += 2;

		// Render description for currently highlighted species
		if (speciesGrid_.GetIndex() >= 0
		    && speciesGrid_.GetIndex()
		           < static_cast<int>(speciesOptions_.size())) {
			const std::string& description =
			    speciesOptions_[speciesGrid_.GetIndex()]
			        .description;
			RenderDescriptionBlock(console, description, leftBound,
			                       rightBound, currentY);
		}
	}

	void CharacterCreationWindow::RenderClassMenu(TCOD_Console* console,
	                                              int /*width*/,
	                                              int /*height*/) const
	{
		// Content bounds: full menu width
		int leftBound = menuPos_.x;
		int rightBound = menuPos_.x + menuWidth_;

		// Render three-column class list
		int currentY = menuStartY_ + 2;
		RenderThreeColumnList(
		    console, classOptions_, classGrid_.GetIndex(),
		    selectedClassIndex_, leftBound, rightBound, currentY);

		// Add line break
		currentY += 2;

		// Render description for currently highlighted class
		if (classGrid_.GetIndex() >= 0
		    && classGrid_.GetIndex()
		           < static_cast<int>(classOptions_.size())) {
			const std::string& description =
			    classOptions_[classGrid_.GetIndex()].description;
			RenderDescriptionBlock(console, description, leftBound,
			                       rightBound, currentY);
		}
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
				speciesGrid_.MoveUp();
				break;

			case CreationTab::Class:
				classGrid_.MoveUp();
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
				speciesGrid_.MoveDown();
				break;

			case CreationTab::Class:
				classGrid_.MoveDown();
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

	void CharacterCreationWindow::SelectLeft()
	{
		switch (currentTab_) {
			case CreationTab::Species:
				speciesGrid_.MoveLeft();
				break;

			case CreationTab::Class:
				classGrid_.MoveLeft();
				break;

			case CreationTab::Stats:
				DecrementStat();
				break;

			case CreationTab::Confirm:
				// No left/right navigation in confirm tab
				break;
		}
	}

	void CharacterCreationWindow::SelectRight()
	{
		switch (currentTab_) {
			case CreationTab::Species:
				speciesGrid_.MoveRight();
				break;

			case CreationTab::Class:
				classGrid_.MoveRight();
				break;

			case CreationTab::Stats:
				IncrementStat();
				break;

			case CreationTab::Confirm:
				// No left/right navigation in confirm tab
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
					speciesGrid_.SetIndex(index);
					selectedSpeciesIndex_ =
					    index; // Mark it
					return true;
				}
				break;

			case CreationTab::Class:
				if (index >= 0
				    && index < static_cast<int>(
				           classOptions_.size())) {
					classGrid_.SetIndex(index);
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
				    == speciesGrid_.GetIndex()) {
					SelectNextTab();
				} else {
					// Mark the selection
					selectedSpeciesIndex_ =
					    speciesGrid_.GetIndex();
				}
				break;

			case CreationTab::Class:
				// If already marked, advance to next tab
				if (selectedClassIndex_
				    == classGrid_.GetIndex()) {
					SelectNextTab();
				} else {
					// Mark the selection
					selectedClassIndex_ =
					    classGrid_.GetIndex();
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
	    int highlightIndex, int selectedIndex, int leftBound,
	    int rightBound, int& currentY) const
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
				// Reserve 5 chars for "*(x) " prefix
				int maxNameWidth = columnWidth - 6;
				std::string displayName = items[itemIndex].name;
				if (static_cast<int>(displayName.length())
				    > maxNameWidth) {
					// HARD CUT: Truncate name without
					// ellipsis
					displayName =
					    displayName.substr(0, maxNameWidth);
				}

				// Add asterisk prefix if this item is confirmed
				std::string prefix =
				    (itemIndex == selectedIndex) ? "*" : " ";
				std::string line = prefix + "("
				                   + std::string(1, letter)
				                   + ") " + displayName;

				// Highlight if this is the currently
				// highlighted item
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

				colY += 1;
				if (colY > maxY) {
					maxY = colY;
				}
			}
		}

		currentY = maxY;
	}

	void CharacterCreationWindow::RenderDescriptionBlock(
	    TCOD_Console* console, const std::string& description,
	    int leftBound, int rightBound, int startY) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();

		int maxWidth = rightBound - leftBound;
		std::string wrappedText = WrapText(description, maxWidth);

		int currentY = startY;
		std::string line;
		for (char c : wrappedText) {
			if (c == '\n') {
				TCOD_printf_rgb(console,
				                (TCOD_PrintParamsRGB) {
				                    .x = leftBound,
				                    .y = currentY,
				                    .width = 0,
				                    .height = 0,
				                    .fg = &textColor,
				                    .bg = NULL,
				                    .flag = TCOD_BKGND_NONE,
				                    .alignment = TCOD_LEFT },
				                "%s", line.c_str());
				line.clear();
				currentY++;
			} else {
				line += c;
			}
		}

		// Print remaining line if any
		if (!line.empty()) {
			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = leftBound,
			                            .y = currentY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &textColor,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", line.c_str());
		}
	}

	std::string CharacterCreationWindow::WrapText(const std::string& text,
	                                              int maxWidth) const
	{
		std::string result;
		std::string currentLine;
		std::string currentWord;

		for (char c : text) {
			if (c == ' ' || c == '\n') {
				// Process the word we've accumulated
				if (!currentWord.empty()) {
					// Check if adding this word would
					// exceed width
					int potentialLength =
					    static_cast<int>(
					        currentLine.length())
					    + (currentLine.empty() ? 0 : 1)
					    + static_cast<int>(
					        currentWord.length());

					if (potentialLength > maxWidth
					    && !currentLine.empty()) {
						// Start new line
						result += currentLine + '\n';
						currentLine = currentWord;
					} else {
						// Add to current line
						if (!currentLine.empty()) {
							currentLine += ' ';
						}
						currentLine += currentWord;
					}

					currentWord.clear();
				}

				// Handle explicit newlines
				if (c == '\n') {
					result += currentLine + '\n';
					currentLine.clear();
				}
			} else {
				currentWord += c;
			}
		}

		// Process final word
		if (!currentWord.empty()) {
			int potentialLength =
			    static_cast<int>(currentLine.length())
			    + (currentLine.empty() ? 0 : 1)
			    + static_cast<int>(currentWord.length());

			if (potentialLength > maxWidth
			    && !currentLine.empty()) {
				result += currentLine + '\n';
				currentLine = currentWord;
			} else {
				if (!currentLine.empty()) {
					currentLine += ' ';
				}
				currentLine += currentWord;
			}
		}

		// Add final line
		if (!currentLine.empty()) {
			result += currentLine;
		}

		return result;
	}

	void CharacterCreationWindow::RenderStatsMenu(TCOD_Console* console,
	                                              int width,
	                                              int height) const
	{
		auto& cfg = ConfigManager::Instance();
		auto textColor = cfg.GetUITextColor();
		auto highlightColor = tcod::ColorRGB { 255, 200, 100 };

		// Show available points
		int currentY = menuStartY_ + 5;
		std::string pointsText =
		    "Points Available: " + std::to_string(availablePoints_);
		int pointsX =
		    width / 2 - static_cast<int>(pointsText.length()) / 2;
		TCOD_printf_rgb(
		    console,
		    (TCOD_PrintParamsRGB) { .x = pointsX,
		                            .y = currentY,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &textColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", pointsText.c_str());

		currentY += 3;

		// Render stat list
		for (std::size_t i = 0; i < stats_.size(); ++i) {
			const auto& stat = stats_[i];
			bool isHighlighted =
			    (static_cast<int>(i) == statsMenuIndex_);

			tcod::ColorRGB color =
			    isHighlighted ? highlightColor : textColor;

			std::string line =
			    stat.name + ": " + std::to_string(stat.value);
			int lineX =
			    width / 2 - static_cast<int>(line.length()) / 2;

			TCOD_printf_rgb(
			    console,
			    (TCOD_PrintParamsRGB) { .x = lineX,
			                            .y = currentY,
			                            .width = 0,
			                            .height = 0,
			                            .fg = &color,
			                            .bg = NULL,
			                            .flag = TCOD_BKGND_NONE,
			                            .alignment = TCOD_LEFT },
			    "%s", line.c_str());

			currentY += 2;
		}

		// Render description for currently highlighted stat
		currentY += 1;
		if (statsMenuIndex_ >= 0
		    && statsMenuIndex_ < static_cast<int>(stats_.size())) {
			const std::string& description =
			    stats_[statsMenuIndex_].description;

			int leftBound = menuPos_.x;
			int rightBound = menuPos_.x + menuWidth_;

			RenderDescriptionBlock(console, description, leftBound,
			                       rightBound, currentY);
		}

		// Render instructions
		currentY = height - 3;
		std::string instructions =
		    "UP/DOWN: Select | LEFT/RIGHT or +/-: Adjust | ENTER: "
		    "Continue";
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
		currentY += 1;
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

		// Render instructions
		currentY = height - 3;
		std::string instructions =
		    "ENTER: Begin Adventure | ESC: Go Back";
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

	bool CharacterCreationWindow::IsReadyToConfirm() const
	{
		return selectedSpeciesIndex_ >= 0 && selectedClassIndex_ >= 0;
	}

} // namespace tutorial
