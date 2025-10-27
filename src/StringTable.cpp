#include "StringTable.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace tutorial
{
	StringTable& StringTable::Instance()
	{
		static StringTable instance;
		return instance;
	}

	void StringTable::LoadLocale(const std::string& locale)
	{
		const std::string filepath = "data/locale/" + locale + ".json";
		std::ifstream file(filepath);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open locale file: "
			                         + filepath);
		}

		try {
			nlohmann::json newData;
			file >> newData;

			// Merge new data into existing locale data
			locale_.merge_patch(newData);

			currentLocale_ = locale;
			std::cout << "[StringTable] Loaded locale: " << locale
			          << std::endl;
		} catch (const nlohmann::json::parse_error& e) {
			throw std::runtime_error("JSON parse error in "
			                         + filepath + ": " + e.what());
		}
	}

	std::string StringTable::GetString(const std::string& key) const
	{
		const nlohmann::json* value = GetNestedValue(key);

		if (!value) {
			std::cerr
			    << "[StringTable] WARNING: Missing string key '"
			    << key << "'" << std::endl;
			return "[MISSING: " + key + "]";
		}

		if (value->is_string()) {
			return value->get<std::string>();
		}

		std::cerr << "[StringTable] WARNING: Key '" << key
		          << "' is not a string" << std::endl;
		return "[INVALID: " + key + "]";
	}

	LocalizedMessage StringTable::GetMessage(
	    const std::string& key,
	    const std::unordered_map<std::string, std::string>& params) const
	{
		const nlohmann::json* value = GetNestedValue(key);

		if (!value) {
			std::cerr
			    << "[StringTable] WARNING: Missing message key '"
			    << key << "'" << std::endl;
			return LocalizedMessage("[MISSING: " + key + "]",
			                        tcod::ColorRGB { 255, 0, 255 },
			                        false);
		}

		// If it's a simple string, return with default color
		if (value->is_string()) {
			std::string text =
			    FormatString(value->get<std::string>(), params);
			return LocalizedMessage(
			    text, tcod::ColorRGB { 255, 255, 255 }, false);
		}

		// If it's an object with text/color/stack fields
		if (value->is_object()) {
			return ParseMessage(*value, params);
		}

		std::cerr << "[StringTable] WARNING: Key '" << key
		          << "' has invalid format" << std::endl;
		return LocalizedMessage("[INVALID: " + key + "]",
		                        tcod::ColorRGB { 255, 0, 255 }, false);
	}

	bool StringTable::Has(const std::string& key) const
	{
		return GetNestedValue(key) != nullptr;
	}

	void StringTable::Clear()
	{
		locale_.clear();
		currentLocale_.clear();
	}

	LocalizedMessage StringTable::ParseMessage(
	    const nlohmann::json& msgJson,
	    const std::unordered_map<std::string, std::string>& params) const
	{
		// Extract text
		std::string text = "";
		if (msgJson.contains("text") && msgJson["text"].is_string()) {
			text = FormatString(msgJson["text"].get<std::string>(),
			                    params);
		}

		// Extract color (default to white)
		tcod::ColorRGB color { 255, 255, 255 };
		if (msgJson.contains("color") && msgJson["color"].is_array()
		    && msgJson["color"].size() == 3) {
			color = tcod::ColorRGB {
				static_cast<uint8_t>(
				    msgJson["color"][0].get<int>()),
				static_cast<uint8_t>(
				    msgJson["color"][1].get<int>()),
				static_cast<uint8_t>(
				    msgJson["color"][2].get<int>())
			};
		}

		// Extract stack flag (default to false)
		bool stack = msgJson.value("stack", false);

		return LocalizedMessage(text, color, stack);
	}

	std::string StringTable::FormatString(
	    const std::string& format,
	    const std::unordered_map<std::string, std::string>& params) const
	{
		std::string result = format;

		// Replace all {key} placeholders with values from params
		for (const auto& [key, value] : params) {
			std::string placeholder = "{" + key + "}";
			size_t pos = 0;

			while ((pos = result.find(placeholder, pos))
			       != std::string::npos) {
				result.replace(pos, placeholder.length(),
				               value);
				pos += value.length();
			}
		}

		return result;
	}

	const nlohmann::json* StringTable::GetNestedValue(
	    const std::string& key) const
	{
		// Split key by dots (e.g., "items.health_potion.name")
		std::vector<std::string> keys;
		std::stringstream ss(key);
		std::string token;

		while (std::getline(ss, token, '.')) {
			keys.push_back(token);
		}

		// Navigate through nested JSON
		const nlohmann::json* current = &locale_;

		for (const auto& k : keys) {
			if (!current->is_object() || !current->contains(k)) {
				return nullptr;
			}
			current = &(*current)[k];
		}

		return current;
	}

} // namespace tutorial
