#include "SpellRegistry.hpp"

#include "Effect.hpp"
#include "TargetSelector.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace tutorial
{
	namespace
	{
		// Helper: Parse effect parameters from JSON
		void ParseEffectParams(const json& params, SpellData& spell)
		{
			if (params.contains("amount")) {
				spell.effectAmounts.push_back(
				    params["amount"].get<int>());
			} else {
				spell.effectAmounts.push_back(0);
			}

			if (params.contains("message")) {
				spell.effectMessages.push_back(
				    params["message"].get<std::string>());
			} else {
				spell.effectMessages.push_back("");
			}

			if (params.contains("radius")) {
				spell.radius = params["radius"].get<float>();
			} else {
				spell.radius = 0.0f;
			}
		}

		// Helper: Set default effect parameters
		void SetDefaultEffectParams(SpellData& spell)
		{
			spell.effectAmounts.push_back(0);
			spell.effectMessages.push_back("");
			spell.radius = 0.0f;
		}

		// Helper: Determine default range based on targeting type
		float GetDefaultRange(const std::string& targetingType)
		{
			if (targetingType == "self") {
				return 0.0f;
			}
			if (targetingType == "area"
			    || targetingType == "single") {
				return 5.0f;
			}
			return 8.0f;
		}

		// Helper: Create a single effect from type, amount, and message
		std::unique_ptr<Effect> CreateSingleEffect(
		    const std::string& type, int amount,
		    const std::string& message)
		{
			if (type == "damage") {
				return std::make_unique<HealthEffect>(-amount,
				                                      message);
			}
			if (type == "health") {
				return std::make_unique<HealthEffect>(amount,
				                                      message);
			}
			if (type == "ai_change") {
				return std::make_unique<AiChangeEffect>(
				    "confused", 10, message);
			}
			throw std::runtime_error("Unknown effect type: "
			                         + type);
		}
	} // namespace

	SpellRegistry& SpellRegistry::Instance()
	{
		static SpellRegistry instance;
		return instance;
	}

	void SpellRegistry::LoadFromDirectory(const std::string& dirPath)
	{
		if (!fs::exists(dirPath)) {
			throw std::runtime_error("Spell directory not found: "
			                         + dirPath);
		}

		std::cout << "[SpellRegistry] Loading spells from " << dirPath
		          << std::endl;

		int loadedCount = 0;
		for (const auto& entry : fs::directory_iterator(dirPath)) {
			if (entry.path().extension() != ".json") {
				continue;
			}

			std::string id = entry.path().stem().string();
			try {
				SpellData spell =
				    LoadSpell(id, entry.path().string());
				spells_[id] = spell;
				loadedCount++;
			} catch (const std::exception& e) {
				std::cerr << "[SpellRegistry] Failed to load "
				             "spell '"
				          << id << "': " << e.what()
				          << std::endl;
			}
		}

		std::cout << "[SpellRegistry] Loaded " << loadedCount
		          << " spells" << std::endl;
	}

	void SpellRegistry::Clear()
	{
		spells_.clear();
	}

	const SpellData* SpellRegistry::Get(const std::string& id) const
	{
		auto it = spells_.find(id);
		if (it != spells_.end()) {
			return &it->second;
		}
		return nullptr;
	}

	bool SpellRegistry::Has(const std::string& id) const
	{
		return spells_.find(id) != spells_.end();
	}

	std::vector<std::string> SpellRegistry::GetAllIds() const
	{
		std::vector<std::string> ids;
		ids.reserve(spells_.size());
		for (const auto& pair : spells_) {
			ids.push_back(pair.first);
		}
		return ids;
	}

	SpellData SpellRegistry::LoadSpell(const std::string& id,
	                                   const std::string& filePath)
	{
		std::ifstream file(filePath);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: "
			                         + filePath);
		}

		json j;
		file >> j;

		SpellData spell;
		spell.id = id;

		// Required: name
		if (!j.contains("name")) {
			throw std::runtime_error("Spell '" + id
			                         + "' missing 'name'");
		}
		spell.name = j["name"];

		// Required: manaCost
		if (!j.contains("manaCost")) {
			throw std::runtime_error("Spell '" + id
			                         + "' missing 'manaCost'");
		}
		spell.manaCost = j["manaCost"];

		// Required: effect
		if (!j.contains("effect")) {
			throw std::runtime_error("Spell '" + id
			                         + "' missing 'effect'");
		}
		std::string effectType = j["effect"];
		spell.effectTypes.push_back(effectType);

		// Parse effect parameters
		if (j.contains("effectParams")) {
			ParseEffectParams(j["effectParams"], spell);
		} else {
			SetDefaultEffectParams(spell);
		}

		// Required: targeting
		if (!j.contains("targeting")) {
			throw std::runtime_error("Spell '" + id
			                         + "' missing 'targeting'");
		}
		spell.targetingType = j["targeting"];

		// Optional: range
		if (j.contains("range")) {
			spell.range = j["range"].get<float>();
		} else {
			spell.range = GetDefaultRange(spell.targetingType);
		}

		return spell;
	}

	std::unique_ptr<TargetSelector> SpellData::CreateTargetSelector() const
	{
		if (targetingType == "self") {
			return std::make_unique<SelfTargetSelector>();
		}
		if (targetingType == "closest_enemy") {
			return std::make_unique<ClosestEnemySelector>(range);
		}
		if (targetingType == "single") {
			return std::make_unique<SingleTargetSelector>(range);
		}
		if (targetingType == "area") {
			return std::make_unique<AreaTargetSelector>(range,
			                                            radius);
		}
		if (targetingType == "beam") {
			return std::make_unique<BeamTargetSelector>(range);
		}
		if (targetingType == "first_in_beam") {
			return std::make_unique<FirstInBeamTargetSelector>(
			    range);
		}

		throw std::runtime_error("Unknown targeting type: "
		                         + targetingType);
	}

	std::vector<std::unique_ptr<Effect>> SpellData::CreateEffects() const
	{
		std::vector<std::unique_ptr<Effect>> effects;

		for (size_t i = 0; i < effectTypes.size(); ++i) {
			const std::string& type = effectTypes[i];
			int amount =
			    (i < effectAmounts.size()) ? effectAmounts[i] : 0;
			std::string message = (i < effectMessages.size())
			                          ? effectMessages[i]
			                          : "";

			effects.push_back(
			    CreateSingleEffect(type, amount, message));
		}

		return effects;
	}

} // namespace tutorial
