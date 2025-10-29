#include "TemplateRegistry.hpp"

#include "Entity.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace tutorial
{
	TemplateRegistry& TemplateRegistry::Instance()
	{
		static TemplateRegistry instance;
		return instance;
	}

	void TemplateRegistry::LoadFromFile(const std::string& filepath)
	{
		// Open file
		std::ifstream file(filepath);
		if (!file.is_open()) {
			throw std::runtime_error(
			    "Failed to open template file: " + filepath);
		}

		// Parse JSON
		json j;
		try {
			file >> j;
		} catch (const json::parse_error& e) {
			throw std::runtime_error("JSON parse error in "
			                         + filepath + ": " + e.what());
		}

		// Each top-level key is a template ID
		for (auto& [id, templateJson] : j.items()) {
			try {
				EntityTemplate tpl =
				    EntityTemplate::FromJson(id, templateJson);
				templates_[id] = tpl; // Last-wins: overwrites
				                      // if ID already exists
				std::cout
				    << "[TemplateRegistry] Loaded template: "
				    << id << std::endl;
			} catch (const std::exception& e) {
				// Continue loading other templates even if one
				// fails
				std::cerr << "[TemplateRegistry] Error loading "
				             "template '"
				          << id << "' from " << filepath << ": "
				          << e.what() << std::endl;
			}
		}
	}

	void TemplateRegistry::LoadFromDirectory(const std::string& directory)
	{
		if (!fs::exists(directory)) {
			throw std::runtime_error("Directory does not exist: "
			                         + directory);
		}

		if (!fs::is_directory(directory)) {
			throw std::runtime_error("Path is not a directory: "
			                         + directory);
		}

		// Iterate through all .json files in directory
		for (const auto& entry : fs::directory_iterator(directory)) {
			if (entry.is_regular_file()
			    && entry.path().extension() == ".json") {
				std::string filepath = entry.path().string();
				std::cout << "[TemplateRegistry] Loading file: "
				          << filepath << std::endl;

				try {
					LoadFromFile(filepath);
				} catch (const std::exception& e) {
					std::cerr << "[TemplateRegistry] "
					             "Failed to load "
					          << filepath << ": "
					          << e.what() << std::endl;
					// Continue loading other files
				}
			}
		}
	}

	void TemplateRegistry::LoadSimplifiedDirectory(
	    const std::string& directory, const std::string& type)
	{
		if (!fs::exists(directory)) {
			throw std::runtime_error("Directory does not exist: "
			                         + directory);
		}

		if (!fs::is_directory(directory)) {
			throw std::runtime_error("Path is not a directory: "
			                         + directory);
		}

		// Validate type parameter
		if (type != "item" && type != "unit") {
			throw std::runtime_error(
			    "Invalid type '" + type
			    + "' - must be 'item' or 'unit'");
		}

		// Iterate through all .json files in directory
		for (const auto& entry : fs::directory_iterator(directory)) {
			if (!entry.is_regular_file()
			    || entry.path().extension() != ".json") {
				continue;
			}

			std::string filepath = entry.path().string();
			std::string id =
			    entry.path().stem().string(); // Filename = ID

			std::cout << "[TemplateRegistry] Loading " << type
			          << ": " << id << " from " << filepath
			          << std::endl;

			try {
				// Open and parse JSON file
				std::ifstream file(filepath);
				if (!file.is_open()) {
					throw std::runtime_error(
					    "Failed to open: " + filepath);
				}

				json j;
				file >> j;

				// Parse based on type and convert to
				// EntityTemplate
				EntityTemplate entityTpl;

				if (type == "item") {
					// Parse as ItemTemplate and convert
					ItemTemplate itemTpl =
					    ItemTemplate::FromJson(id, j);

					// Convert ItemTemplate ->
					// EntityTemplate
					entityTpl.id = itemTpl.id;
					entityTpl.name = itemTpl.name;
					entityTpl.pluralName =
					    itemTpl.pluralName;
					entityTpl.icon = itemTpl.icon;
					entityTpl.color = itemTpl.color;
					entityTpl.blocks = false;
					entityTpl.faction = "neutral";
					entityTpl.hp = 1;
					entityTpl.maxHp = 1;
					entityTpl.defense = 0;
					entityTpl.power = 0;
					entityTpl.xpReward = 0;
					entityTpl.pickable = true;

					// Convert item data to ItemData format
					ItemData itemData;
					itemData.targeting.type =
					    itemTpl.targetingType;
					itemData.targeting.range =
					    itemTpl.range;
					itemData.targeting.radius =
					    itemTpl.radius;

					// Convert effects JSON to EffectData
					for (const auto& effectJson :
					     itemTpl.effects) {
						EffectData effectData;
						effectData.type =
						    effectJson["type"];

						if (effectJson.contains(
						        "amount")) {
							effectData.amount =
							    effectJson
							        ["amount"];
						}
						if (effectJson.contains("ai")) {
							effectData.aiType =
							    effectJson["ai"];
						}
						if (effectJson.contains(
						        "duration")) {
							effectData.duration =
							    effectJson
							        ["duration"];
						}
						if (effectJson.contains(
						        "message")) {
							effectData.messageKey =
							    effectJson
							        ["message"];
						}

						itemData.effects.push_back(
						    effectData);
					}

					entityTpl.item = itemData;

				} else if (type == "unit") {
					// Parse as UnitTemplate and convert
					UnitTemplate unitTpl =
					    UnitTemplate::FromJson(id, j);

					// Convert UnitTemplate ->
					// EntityTemplate
					entityTpl.id = unitTpl.id;
					entityTpl.name = unitTpl.name;
					entityTpl.pluralName =
					    unitTpl.pluralName;
					entityTpl.icon = unitTpl.icon;
					entityTpl.color = unitTpl.color;
					entityTpl.blocks = unitTpl.blocks;
					entityTpl.faction = "monster";
					entityTpl.hp = unitTpl.hp;
					entityTpl.maxHp = unitTpl.hp;
					entityTpl.defense = unitTpl.defense;
					entityTpl.power = unitTpl.power;
					entityTpl.xpReward = unitTpl.xp;
					entityTpl.aiType = unitTpl.ai;
					entityTpl.pickable = false;
				}

				// Store in templates map
				templates_[id] = entityTpl;

				std::cout << "[TemplateRegistry] Loaded "
				          << type << " template: " << id
				          << std::endl;

			} catch (const std::exception& e) {
				std::cerr << "[TemplateRegistry] Error loading "
				          << type << " '" << id << "' from "
				          << filepath << ": " << e.what()
				          << std::endl;
				// Continue loading other templates
			}
		}
	}

	const EntityTemplate* TemplateRegistry::Get(const std::string& id) const
	{
		auto it = templates_.find(id);
		if (it != templates_.end()) {
			return &it->second;
		}
		return nullptr;
	}

	bool TemplateRegistry::Has(const std::string& id) const
	{
		return templates_.find(id) != templates_.end();
	}

	std::unique_ptr<Entity> TemplateRegistry::Create(const std::string& id,
	                                                 pos_t pos) const
	{
		const EntityTemplate* tpl = Get(id);
		if (!tpl) {
			throw std::runtime_error("Template not found: " + id);
		}
		return tpl->CreateEntity(pos);
	}

	void TemplateRegistry::Clear()
	{
		templates_.clear();
		std::cout << "[TemplateRegistry] Cleared all templates"
		          << std::endl;
	}

	std::vector<std::string> TemplateRegistry::GetAllIds() const
	{
		std::vector<std::string> ids;
		ids.reserve(templates_.size());
		for (const auto& [id, tpl] : templates_) {
			ids.push_back(id);
		}
		return ids;
	}
} // namespace tutorial
