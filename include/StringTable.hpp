#ifndef STRING_TABLE_HPP
#define STRING_TABLE_HPP

#include <libtcod/color.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

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

	class StringTable
	{
	public:
		// Get singleton instance
		static StringTable& Instance();

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

	private:
		StringTable() = default;
		~StringTable() = default;

		// Prevent copying
		StringTable(const StringTable&) = delete;
		StringTable& operator=(const StringTable&) = delete;

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
	};
} // namespace tutorial

#endif // STRING_TABLE_HPP
