#ifndef CHARACTER_CREATION_WINDOW_HPP
#define CHARACTER_CREATION_WINDOW_HPP

#include "Position.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace tutorial
{
	enum class CreationTab {
		Species = 0,
		Class = 1,
		Stats = 2,
		Confirm = 3
	};

	struct CreationOption {
		std::string id;
		std::string name;
		std::string description;
	};

	struct StatValue {
		std::string id;
		std::string name;
		std::string description;
		int value;
	};

	// Window for character creation with tabs
	class CharacterCreationWindow : public UiWindowBase
	{
	public:
		CharacterCreationWindow(std::size_t width, std::size_t height,
		                        pos_t pos);

		void Render(TCOD_Console* parent) const override;

		// Tab navigation
		void SelectNextTab();
		void SelectPreviousTab();
		void SelectTab(CreationTab tab);
		CreationTab GetCurrentTab() const
		{
			return currentTab_;
		}

		// Menu navigation (within current tab)
		void SelectPrevious();
		void SelectNext();
		bool SelectByLetter(char letter);

		// Confirm current selection (for Species/Class tabs)
		void ConfirmSelection();

		// Get current selections
		int GetSelectedSpeciesIndex() const
		{
			return selectedSpeciesIndex_;
		}
		int GetSelectedClassIndex() const
		{
			return selectedClassIndex_;
		}

		// Stats management (for Stats tab)
		void IncrementStat();
		void DecrementStat();
		const std::vector<StatValue>& GetStats() const
		{
			return stats_;
		}

		// Check if ready to confirm
		bool IsReadyToConfirm() const;

	private:
		void LoadSpeciesOptions();
		void LoadClassOptions();
		void InitializeStats();

		void RenderTabs(TCOD_Console* console, int width) const;
		void RenderSpeciesMenu(TCOD_Console* console, int width,
		                       int height) const;
		void RenderClassMenu(TCOD_Console* console, int width,
		                     int height) const;
		void RenderStatsMenu(TCOD_Console* console, int width,
		                     int height) const;
		void RenderConfirmMenu(TCOD_Console* console, int width,
		                       int height) const;

		CreationTab currentTab_;

		// Species options
		std::vector<CreationOption> speciesOptions_;
		int speciesMenuIndex_;
		int selectedSpeciesIndex_;

		// Class options
		std::vector<CreationOption> classOptions_;
		int classMenuIndex_;
		int selectedClassIndex_;

		// Stats
		std::vector<StatValue> stats_;
		int statsMenuIndex_;
		int availablePoints_;

		// Menu positioning
		int menuStartY_;
		int menuWidth_;
		int menuHeight_;
		pos_t menuPos_;
	};

} // namespace tutorial

#endif // CHARACTER_CREATION_WINDOW_HPP
