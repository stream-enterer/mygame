#ifndef LOCALEMANAGER_HPP
#define LOCALEMANAGER_HPP

#include <libtcod/color.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace tutorial
{
	// Structure for a localized message with color and stacking info
	struct LocalizedMessage {
		std::string text;
		tcod::ColorRGB color;
		bool stack;

		LocalizedMessage()
		    : text(""),
		      color(tcod::ColorRGB { 255, 255, 255 }),
		      stack(false)
		{
		}

		LocalizedMessage(const std::string& t, tcod::ColorRGB c, bool s)
		    : text(t), color(c), stack(s)
		{
		}
	};

	// Species and class data structures
	struct SpeciesClassData {
		int order;
		std::string name;
		std::string description;
	};

	class LocaleManager
	{
	public:
		// Get singleton instance
		static LocaleManager& Instance();

		// Load a locale file (e.g., "en_US")
		void LoadLocale(const std::string& locale);

		// Get a simple string by key (e.g., "ui.inventory_title")
		std::string GetString(const std::string& key) const;

		// Get a localized message with color and formatting
		// params: map of placeholder names to values, e.g., {"amount",
		// "5"}
		LocalizedMessage GetMessage(
		    const std::string& key,
		    const std::unordered_map<std::string, std::string>&
		        params = {}) const;

		// Check if a key exists
		bool Has(const std::string& key) const;

		// Get current locale name
		const std::string& GetCurrentLocale() const
		{
			return currentLocale_;
		}

		// Clear all loaded strings
		void Clear();

		// Get species and class data
		const std::vector<SpeciesClassData>& GetSpecies() const
		{
			return species_;
		}
		const std::vector<SpeciesClassData>& GetClasses() const
		{
			return classes_;
		}

	private:
		LocaleManager() = default;
		~LocaleManager() = default;

		// Prevent copying
		LocaleManager(const LocaleManager&) = delete;
		LocaleManager& operator=(const LocaleManager&) = delete;

		// Parse a message JSON object (handles text, color, stack
		// fields)
		LocalizedMessage ParseMessage(
		    const nlohmann::json& msgJson,
		    const std::unordered_map<std::string, std::string>& params)
		    const;

		// Format a string by replacing {placeholders} with values
		std::string FormatString(
		    const std::string& format,
		    const std::unordered_map<std::string, std::string>& params)
		    const;

		// Navigate nested JSON using dot notation (e.g.,
		// "items.health_potion.name")
		const nlohmann::json* GetNestedValue(
		    const std::string& key) const;

		nlohmann::json locale_;
		std::string currentLocale_;
		std::vector<SpeciesClassData> species_;
		std::vector<SpeciesClassData> classes_;
	};
} // namespace tutorial

#endif // LOCALEMANAGER_HPP
