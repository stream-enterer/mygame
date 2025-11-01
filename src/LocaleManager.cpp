#include "LocaleManager.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace tutorial
{
	LocaleManager& LocaleManager::Instance()
	{
		static LocaleManager instance;
		return instance;
	}

	void LocaleManager::LoadLocale(const std::string& locale)
	{
		const std::string filepath =
		    "data/locale/" + locale + "/strings." + locale + ".json";
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
			std::cout << "[LocaleManager] Loaded locale: " << locale
			          << std::endl;
		} catch (const nlohmann::json::parse_error& e) {
			throw std::runtime_error("JSON parse error in "
			                         + filepath + ": " + e.what());
		}

		// Load and validate species
		std::string speciesPath =
		    "data/locale/" + locale + "/species." + locale + ".json";
		std::ifstream speciesFile(speciesPath);
		if (!speciesFile.is_open()) {
			throw std::runtime_error(
			    "[LocaleManager] Failed to open species file: "
			    + speciesPath);
		}

		nlohmann::json speciesJson;
		try {
			speciesFile >> speciesJson;
		} catch (const nlohmann::json::exception& e) {
			throw std::runtime_error(
			    "[LocaleManager] JSON parse error in " + speciesPath
			    + ": " + e.what());
		}

		if (!speciesJson.contains("species")
		    || !speciesJson["species"].is_array()) {
			throw std::runtime_error(
			    "[LocaleManager] Invalid species.json: missing or "
			    "invalid 'species' array");
		}

		species_.clear();
		for (const auto& entry : speciesJson["species"]) {
			if (!entry.contains("order") || !entry.contains("name")
			    || !entry.contains("description")) {
				throw std::runtime_error(
				    "[LocaleManager] Invalid species entry: "
				    "missing required fields (order, name, "
				    "description)");
			}

			SpeciesClassData data;
			data.order = entry["order"].get<int>();
			data.name = entry["name"].get<std::string>();
			data.description =
			    entry["description"].get<std::string>();
			species_.push_back(data);
		}

		if (species_.empty()) {
			throw std::runtime_error(
			    "[LocaleManager] Species validation failed: must "
			    "have at least 1 species");
		}
		if (species_.size() > 12) {
			throw std::runtime_error(
			    "[LocaleManager] Species validation failed: cannot "
			    "have more than 12 species (found "
			    + std::to_string(species_.size()) + ")");
		}

		// Sort by order
		std::sort(
		    species_.begin(), species_.end(),
		    [](const SpeciesClassData& a, const SpeciesClassData& b) {
			    return a.order < b.order;
		    });

		std::cout << "[LocaleManager] Loaded " << species_.size()
		          << " species" << std::endl;

		// Load and validate classes
		std::string classPath =
		    "data/locale/" + locale + "/class." + locale + ".json";
		std::ifstream classFile(classPath);
		if (!classFile.is_open()) {
			throw std::runtime_error(
			    "[LocaleManager] Failed to open class file: "
			    + classPath);
		}

		nlohmann::json classJson;
		try {
			classFile >> classJson;
		} catch (const nlohmann::json::exception& e) {
			throw std::runtime_error(
			    "[LocaleManager] JSON parse error in " + classPath
			    + ": " + e.what());
		}

		if (!classJson.contains("classes")
		    || !classJson["classes"].is_array()) {
			throw std::runtime_error(
			    "[LocaleManager] Invalid class.json: missing or "
			    "invalid 'classes' array");
		}

		classes_.clear();
		for (const auto& entry : classJson["classes"]) {
			if (!entry.contains("order") || !entry.contains("name")
			    || !entry.contains("description")) {
				throw std::runtime_error(
				    "[LocaleManager] Invalid class entry: "
				    "missing required fields (order, name, "
				    "description)");
			}

			SpeciesClassData data;
			data.order = entry["order"].get<int>();
			data.name = entry["name"].get<std::string>();
			data.description =
			    entry["description"].get<std::string>();
			classes_.push_back(data);
		}

		if (classes_.empty()) {
			throw std::runtime_error(
			    "[LocaleManager] Class validation failed: must "
			    "have at least 1 class");
		}
		if (classes_.size() > 12) {
			throw std::runtime_error(
			    "[LocaleManager] Class validation failed: cannot "
			    "have more than 12 classes (found "
			    + std::to_string(classes_.size()) + ")");
		}

		// Sort by order
		std::sort(
		    classes_.begin(), classes_.end(),
		    [](const SpeciesClassData& a, const SpeciesClassData& b) {
			    return a.order < b.order;
		    });

		std::cout << "[LocaleManager] Loaded " << classes_.size()
		          << " classes" << std::endl;
	}

	std::string LocaleManager::GetString(const std::string& key) const
	{
		const nlohmann::json* value = GetNestedValue(key);

		if (!value) {
			std::cerr
			    << "[LocaleManager] WARNING: Missing string key '"
			    << key << "'" << std::endl;
			return "[MISSING: " + key + "]";
		}

		if (value->is_string()) {
			return value->get<std::string>();
		}

		std::cerr << "[LocaleManager] WARNING: Key '" << key
		          << "' is not a string" << std::endl;
		return "[INVALID: " + key + "]";
	}

	LocalizedMessage LocaleManager::GetMessage(
	    const std::string& key,
	    const std::unordered_map<std::string, std::string>& params) const
	{
		const nlohmann::json* value = GetNestedValue(key);

		if (!value) {
			std::cerr
			    << "[LocaleManager] WARNING: Missing message key '"
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

		std::cerr << "[LocaleManager] WARNING: Key '" << key
		          << "' has invalid format" << std::endl;
		return LocalizedMessage("[INVALID: " + key + "]",
		                        tcod::ColorRGB { 255, 0, 255 }, false);
	}

	bool LocaleManager::Has(const std::string& key) const
	{
		return GetNestedValue(key) != nullptr;
	}

	void LocaleManager::Clear()
	{
		locale_.clear();
		currentLocale_.clear();
	}

	LocalizedMessage LocaleManager::ParseMessage(
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

	std::string LocaleManager::FormatString(
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

	const nlohmann::json* LocaleManager::GetNestedValue(
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
