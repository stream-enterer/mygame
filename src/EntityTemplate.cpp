#include "EntityTemplate.hpp"

#include "AiComponent.hpp"
#include "Colors.hpp"
#include "Components.hpp"
#include "Effect.hpp"
#include "Entity.hpp"
#include "Item.hpp"
#include "Position.hpp"
#include "TargetSelector.hpp"

#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace tutorial
{

	SpawnData SpawnData::FromJson(const json& j)
	{
		SpawnData data;

		if (!j.contains("location")) {
			throw std::runtime_error(
			    "Spawn data missing required 'location' field");
		}
		data.location = j["location"];

		if (!j.contains("weight")) {
			throw std::runtime_error(
			    "Spawn data missing required 'weight' field");
		}
		data.weight = j["weight"];

		return data;
	}

	ItemTemplate ItemTemplate::FromJson(const std::string& id,
	                                    const json& j)
	{
		ItemTemplate tpl;
		tpl.id = id;

		// Required: name
		if (!j.contains("name")) {
			throw std::runtime_error("Item '" + id
			                         + "' missing 'name'");
		}
		tpl.name = j["name"];

		// Optional: plural name (defaults to name + "s")
		if (j.contains("pluralName")) {
			tpl.pluralName = j["pluralName"];
		} else {
			tpl.pluralName = tpl.name + "s";
		}

		// Required: char
		if (!j.contains("char") || !j["char"].is_string()) {
			throw std::runtime_error("Item '" + id
			                         + "' missing 'char'");
		}
		std::string charStr = j["char"];
		tpl.icon = charStr.empty() ? '?' : charStr[0];

		// Required: color [r, g, b]
		if (!j.contains("color") || !j["color"].is_array()
		    || j["color"].size() != 3) {
			throw std::runtime_error(
			    "Item '" + id + "' missing valid 'color' [r,g,b]");
		}
		tpl.color =
		    tcod::ColorRGB { static_cast<uint8_t>(j["color"][0]),
			             static_cast<uint8_t>(j["color"][1]),
			             static_cast<uint8_t>(j["color"][2]) };

		// Required: targeting
		if (!j.contains("targeting")) {
			throw std::runtime_error("Item '" + id
			                         + "' missing 'targeting'");
		}
		tpl.targetingType = j["targeting"];

		// Optional: range and radius
		if (j.contains("range")) {
			tpl.range = j["range"];
		}
		if (j.contains("radius")) {
			tpl.radius = j["radius"];
		}

		// Required: effects array
		if (!j.contains("effects") || !j["effects"].is_array()) {
			throw std::runtime_error("Item '" + id
			                         + "' missing 'effects' array");
		}
		tpl.effects = j["effects"];

		return tpl;
	}

	std::unique_ptr<Entity> ItemTemplate::CreateEntity(pos_t pos) const
	{
		// Create target selector based on targeting type
		std::unique_ptr<TargetSelector> selector;

		if (targetingType == "self") {
			selector = std::make_unique<SelfTargetSelector>();
		} else if (targetingType == "closest_enemy") {
			float r = range.value_or(5.0f);
			selector = std::make_unique<ClosestEnemySelector>(r);
		} else if (targetingType == "single") {
			float r = range.value_or(8.0f);
			selector = std::make_unique<SingleTargetSelector>(r);
		} else if (targetingType == "area") {
			float r = range.value_or(3.0f);
			float rad = radius.value_or(3.0f);
			selector = std::make_unique<AreaTargetSelector>(r, rad);
		} else if (targetingType == "beam") {
			float r = range.value_or(8.0f);
			selector = std::make_unique<BeamTargetSelector>(r);
		} else if (targetingType == "first_in_beam") {
			float r = range.value_or(8.0f);
			selector =
			    std::make_unique<FirstInBeamTargetSelector>(r);
		} else {
			throw std::runtime_error("Unknown targeting type: "
			                         + targetingType);
		}

		// Parse effects from stored JSON
		std::vector<std::unique_ptr<Effect>> effectList;

		for (const auto& effectJson : effects) {
			if (!effectJson.contains("type")) {
				throw std::runtime_error(
				    "Effect missing 'type' field");
			}

			std::string type = effectJson["type"];

			if (type == "health") {
				int amount = effectJson.value("amount", 0);
				std::string msg =
				    effectJson.value("message", "");
				effectList.push_back(
				    std::make_unique<HealthEffect>(amount,
				                                   msg));
			} else if (type == "ai_change") {
				std::string ai =
				    effectJson.value("ai", "confused");
				int duration = effectJson.value("duration", 10);
				std::string msg =
				    effectJson.value("message", "");
				effectList.push_back(
				    std::make_unique<AiChangeEffect>(
				        ai, duration, msg));
			} else {
				throw std::runtime_error("Unknown effect type: "
				                         + type);
			}
		}

		// Create item component
		auto itemComponent = std::make_unique<Item>(
		    std::move(selector), std::move(effectList));

		// Create entity with item component
		return std::make_unique<BaseEntity>(
		    pos, name,
		    false,                   // Items don't block movement
		    AttackerComponent { 0 }, // Items don't attack
		    DestructibleComponent { 0, 1, 1 }, // Minimal stats
		    IconRenderable { color, icon }, Faction::NEUTRAL,
		    std::move(itemComponent),
		    true // Pickable
		);
	}

	UnitTemplate UnitTemplate::FromJson(const std::string& id,
	                                    const json& j)
	{
		UnitTemplate tpl;
		tpl.id = id;

		// Required: name
		if (!j.contains("name")) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'name'");
		}
		tpl.name = j["name"];

		// Optional: plural name (defaults to name + "s")
		if (j.contains("pluralName")) {
			tpl.pluralName = j["pluralName"];
		} else {
			tpl.pluralName = tpl.name + "s";
		}

		// Required: char
		if (!j.contains("char") || !j["char"].is_string()) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'char'");
		}
		std::string charStr = j["char"];
		tpl.icon = charStr.empty() ? '?' : charStr[0];

		// Required: color [r, g, b]
		if (!j.contains("color") || !j["color"].is_array()
		    || j["color"].size() != 3) {
			throw std::runtime_error(
			    "Unit '" + id + "' missing valid 'color' [r,g,b]");
		}
		tpl.color =
		    tcod::ColorRGB { static_cast<uint8_t>(j["color"][0]),
			             static_cast<uint8_t>(j["color"][1]),
			             static_cast<uint8_t>(j["color"][2]) };

		// Required: blocks
		if (!j.contains("blocks")) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'blocks'");
		}
		tpl.blocks = j["blocks"];

		// Required: combat stats
		if (!j.contains("hp")) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'hp'");
		}
		tpl.hp = j["hp"];

		if (!j.contains("defense")) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'defense'");
		}
		tpl.defense = j["defense"];

		if (!j.contains("power")) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'power'");
		}
		tpl.power = j["power"];

		// XP reward (defaults to 0)
		tpl.xp = j.value("xp", 0);

		// Required: AI type
		if (!j.contains("ai")) {
			throw std::runtime_error("Unit '" + id
			                         + "' missing 'ai'");
		}
		tpl.ai = j["ai"];

		return tpl;
	}

	std::unique_ptr<Entity> UnitTemplate::CreateEntity(pos_t pos) const
	{
		// Create AI component
		std::unique_ptr<AiComponent> aiComponent = nullptr;
		if (ai == "hostile") {
			aiComponent = std::make_unique<HostileAi>();
		} else {
			throw std::runtime_error("Unknown AI type: " + ai);
		}

		// Create destructible component with XP reward
		DestructibleComponent destructible {
			static_cast<unsigned int>(defense),
			static_cast<unsigned int>(hp),
			static_cast<unsigned int>(hp) // hp = maxHp
		};
		destructible.SetXpReward(static_cast<unsigned int>(xp));

		// Create monster entity
		return std::make_unique<Npc>(
		    pos, name, blocks,
		    AttackerComponent { static_cast<unsigned int>(power) },
		    destructible, IconRenderable { color, icon },
		    Faction::MONSTER, std::move(aiComponent),
		    false // Not pickable
		);
	}

	EntityTemplate EntityTemplate::FromJson(const std::string& id,
	                                        const json& j)
	{
		EntityTemplate tpl;

		tpl.id = id;

		// Required fields - throw if missing
		if (!j.contains("name")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'name'");
		}
		tpl.name = j["name"];

		// Parse optional plural name (defaults to name + "s" if not
		// specified)
		if (j.contains("pluralName")) {
			tpl.pluralName = j["pluralName"];
		} else {
			tpl.pluralName = tpl.name + "s";
		}

		if (!j.contains("char")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'char'");
		}
		std::string charStr = j["char"];
		if (charStr.empty()) {
			throw std::runtime_error("Entity '" + id
			                         + "' has empty 'char' field");
		}
		tpl.icon = charStr[0];

		if (!j.contains("color") || !j["color"].is_array()
		    || j["color"].size() != 3) {
			throw std::runtime_error(
                "Entity '" + id
                + "' missing or invalid 'color' field (must be [r,g,b])");
		}
		tpl.color = tcod::ColorRGB {
			static_cast<uint8_t>(j["color"][0].get<int>()),
			static_cast<uint8_t>(j["color"][1].get<int>()),
			static_cast<uint8_t>(j["color"][2].get<int>())
		};

		if (!j.contains("blocks")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'blocks'");
		}
		tpl.blocks = j["blocks"];

		if (!j.contains("faction")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'faction'");
		}
		tpl.faction = j["faction"];

		if (!j.contains("hp")) {
			throw std::runtime_error(
			    "Entity '" + id + "' missing required field 'hp'");
		}
		tpl.hp = j["hp"];

		if (!j.contains("maxHp")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'maxHp'");
		}
		tpl.maxHp = j["maxHp"];

		if (!j.contains("defense")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'defense'");
		}
		tpl.defense = j["defense"];

		if (!j.contains("power")) {
			throw std::runtime_error(
			    "Entity '" + id
			    + "' missing required field 'power'");
		}
		tpl.power = j["power"];

		// Parse xpReward (optional, defaults to 0)
		tpl.xpReward = j.value("xpReward", 0);

		// AI is optional (items don't need it)
		if (j.contains("ai")) {
			tpl.aiType = j["ai"];
		}

		// Pickable flag (defaults to true if not specified)
		if (j.contains("pickable")) {
			tpl.pickable = j["pickable"];
		}

		// Corpse flag (defaults to false if not specified)
		if (j.contains("isCorpse")) {
			tpl.isCorpse = j["isCorpse"];
		}

		// Optional spawn data
		if (j.contains("spawns") && j["spawns"].is_array()) {
			for (const auto& spawnJson : j["spawns"]) {
				try {
					tpl.spawns.push_back(
					    SpawnData::FromJson(spawnJson));
				} catch (const std::exception& e) {
					std::cerr
					    << "[EntityTemplate] Warning: "
					       "Invalid spawn data for '"
					    << id << "': " << e.what()
					    << std::endl;
				}
			}
		}

		// Validation: Monsters must have AI
		if (tpl.faction == "monster" && !tpl.aiType.has_value()) {
			throw std::runtime_error(
                "Monster template '" + id
                + "' must have 'ai' field (monsters require AI behavior)");
		}

		return tpl;
	}

	json EntityTemplate::ToJson() const
	{
		json j;

		j["name"] = name;
		j["char"] = std::string(1, icon);
		j["color"] = json::array({ color.r, color.g, color.b });
		j["blocks"] = blocks;
		j["faction"] = faction;
		j["hp"] = hp;
		j["maxHp"] = maxHp;
		j["defense"] = defense;
		j["power"] = power;
		if (aiType.has_value()) {
			j["ai"] = aiType.value();
		}

		// Serialize spawn data
		if (!spawns.empty()) {
			json spawnsArray = json::array();
			for (const auto& spawn : spawns) {
				json spawnJson;
				spawnJson["location"] = spawn.location;
				spawnJson["weight"] = spawn.weight;
				spawnsArray.push_back(spawnJson);
			}
			j["spawns"] = spawnsArray;
		}

		return j;
	}

	std::unique_ptr<Entity> EntityTemplate::CreateEntity(pos_t pos) const
	{
		// Determine faction enum
		Faction factionEnum;
		if (faction == "player") {
			factionEnum = Faction::PLAYER;
		} else if (faction == "monster") {
			factionEnum = Faction::MONSTER;
		} else {
			factionEnum = Faction::NEUTRAL;
		}

		// Create item component if this template has item data
		std::unique_ptr<Item> itemComponent = nullptr;
		if (item.has_value()) {
			const auto& itemData = item.value();

			// Create target selector
			std::unique_ptr<TargetSelector> selector;
			const auto& targeting = itemData.targeting;

			if (targeting.type == "self") {
				selector =
				    std::make_unique<SelfTargetSelector>();
			} else if (targeting.type == "closest_enemy") {
				float range = targeting.range.value_or(5.0f);
				selector =
				    std::make_unique<ClosestEnemySelector>(
				        range);
			} else if (targeting.type == "single") {
				float range = targeting.range.value_or(8.0f);
				selector =
				    std::make_unique<SingleTargetSelector>(
				        range);
			} else if (targeting.type == "area") {
				float range = targeting.range.value_or(3.0f);
				float radius = targeting.radius.value_or(3.0f);
				selector = std::make_unique<AreaTargetSelector>(
				    range, radius);
			} else if (targeting.type == "beam") {
				float range = targeting.range.value_or(8.0f);
				selector =
				    std::make_unique<BeamTargetSelector>(range);
			} else if (targeting.type == "first_in_beam") {
				float range = targeting.range.value_or(8.0f);
				selector =
				    std::make_unique<FirstInBeamTargetSelector>(
				        range);
			} else {
				throw std::runtime_error(
				    "Unknown targeting type: "
				    + targeting.type);
			}

			// Create effects
			std::vector<std::unique_ptr<Effect>> effects;
			for (const auto& effectData : itemData.effects) {
				if (effectData.type == "health") {
					int amount =
					    effectData.amount.value_or(0);
					std::string msgKey =
					    effectData.messageKey.value_or(
					        std::string());
					effects.push_back(
					    std::make_unique<HealthEffect>(
					        amount, msgKey));
				} else if (effectData.type == "ai_change") {
					std::string aiType =
					    effectData.aiType.value_or(
					        "confused");
					int duration =
					    effectData.duration.value_or(10);
					std::string msgKey =
					    effectData.messageKey.value_or(
					        std::string());
					effects.push_back(
					    std::make_unique<AiChangeEffect>(
					        aiType, duration, msgKey));
				} else {
					throw std::runtime_error(
					    "Unknown effect type: "
					    + effectData.type);
				}
			}

			itemComponent = std::make_unique<Item>(
			    std::move(selector), std::move(effects));
		}

		// Create AI component based on type (if AI type is specified)
		std::unique_ptr<AiComponent> aiComponent = nullptr;
		if (aiType.has_value()) {
			if (aiType.value() == "hostile") {
				aiComponent = std::make_unique<HostileAi>();
			} else if (aiType.value() == "player") {
				// Player AI will be set by Player class itself
				aiComponent = nullptr;
			}
		}

		// Create appropriate entity type
		if (factionEnum == Faction::PLAYER) {
			auto entity = std::make_unique<Player>(
			    pos, name, blocks,
			    AttackerComponent {
			        static_cast<unsigned int>(power) },
			    DestructibleComponent {
			        static_cast<unsigned int>(defense),
			        static_cast<unsigned int>(maxHp),
			        static_cast<unsigned int>(hp) },
			    IconRenderable { color, icon }, factionEnum,
			    pickable);
			entity->SetPluralName(pluralName);
			entity->SetTemplateId(id);
			return entity;
		} else if (aiComponent != nullptr) {
			// Monster with AI - create destructible with XP reward
			DestructibleComponent destructible {
				static_cast<unsigned int>(defense),
				static_cast<unsigned int>(maxHp),
				static_cast<unsigned int>(hp)
			};
			destructible.SetXpReward(
			    static_cast<unsigned int>(xpReward));

			auto entity = std::make_unique<Npc>(
			    pos, name, blocks,
			    AttackerComponent {
			        static_cast<unsigned int>(power) },
			    destructible, IconRenderable { color, icon },
			    factionEnum, std::move(aiComponent), pickable,
			    isCorpse);
			entity->SetPluralName(pluralName);
			entity->SetTemplateId(id);
			return entity;
		} else {
			// Item or neutral entity without AI
			auto entity = std::make_unique<BaseEntity>(
			    pos, name, blocks,
			    AttackerComponent {
			        static_cast<unsigned int>(power) },
			    DestructibleComponent {
			        static_cast<unsigned int>(defense),
			        static_cast<unsigned int>(maxHp),
			        static_cast<unsigned int>(hp) },
			    IconRenderable { color, icon }, factionEnum,
			    std::move(itemComponent), pickable, isCorpse);
			entity->SetPluralName(pluralName);
			entity->SetTemplateId(id);
			return entity;
		}
	}
} // namespace tutorial
