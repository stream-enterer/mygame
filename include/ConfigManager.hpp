#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <libtcod/color.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace tutorial
{
	class ConfigManager
	{
	public:
		// Get singleton instance
		static ConfigManager& Instance();

		// Load all configuration files
		void LoadAll();

		// Clear all configs (useful for testing)
		void Clear();

		// === Game Config Accessors ===
		int GetPlayerFOVRadius() const;
		int GetMaxInventorySize() const;
		float GetDifficultyMultiplier() const;

		// Debug flags
		bool IsDebugShowAllMap() const;
		bool IsDebugInvincible() const;
		bool IsDebugLogAI() const;

		// === UI Config Accessors ===
		int GetMapHeightOffset() const;

		// Health bar
		int GetHealthBarWidth() const;
		int GetHealthBarHeight() const;
		int GetHealthBarX() const;
		int GetHealthBarY() const;
		tcod::ColorRGB GetHealthBarFullColor() const;
		tcod::ColorRGB GetHealthBarEmptyColor() const;
		tcod::ColorRGB GetXpBarFullColor() const;
		tcod::ColorRGB GetXpBarEmptyColor() const;

		// Message log
		int GetMessageLogWidth() const;
		int GetMessageLogHeight() const;
		int GetMessageLogX() const;
		int GetMessageLogY() const;

		// Inventory window
		int GetInventoryWindowWidth() const;
		int GetInventoryWindowHeight() const;
		bool GetInventoryCenterOnScreen() const;

		// UI colors
		tcod::ColorRGB GetUIFrameColor() const;
		tcod::ColorRGB GetUITextColor() const;

		// === Raw JSON Access (for custom queries) ===
		const nlohmann::json& GetGameConfig() const
		{
			return gameConfig_;
		}
		const nlohmann::json& GetUIConfig() const
		{
			return uiConfig_;
		}

	private:
		ConfigManager() = default;
		~ConfigManager() = default;

		// Prevent copying
		ConfigManager(const ConfigManager&) = delete;
		ConfigManager& operator=(const ConfigManager&) = delete;

		void LoadGameConfig();
		void LoadUIConfig();

		nlohmann::json gameConfig_;
		nlohmann::json uiConfig_;
	};
} // namespace tutorial

#endif // CONFIG_MANAGER_HPP
