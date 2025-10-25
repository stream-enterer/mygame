#ifndef ENTITY_TEMPLATE_HPP
#define ENTITY_TEMPLATE_HPP

#include <libtcod/color.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace tutorial
{
	// Forward declarations
	class Entity;
	struct pos_t;

	// Spawn location data parsed from JSON
	struct SpawnData {
		std::string location; // e.g., "dungeon_1", "cave", "swamp"
		int weight;           // Spawn weight (higher = more common)

		static SpawnData FromJson(const nlohmann::json& j);
	};

	// Effect data parsed from JSON
	struct EffectData {
		std::string type;                      // "health", "ai_change"
		std::optional<int> amount;             // For health effects
		std::optional<std::string> aiType;     // For AI change effects
		std::optional<int> duration;           // For AI change effects
		std::optional<std::string> messageKey; // Localization key

		static EffectData FromJson(const nlohmann::json& j);
	};

	// Target selector data parsed from JSON
	struct TargetingData {
		std::string type; // "self", "closest_enemy", "single", "area"
		std::optional<float> range;  // For ranged selectors
		std::optional<float> radius; // For area selectors

		static TargetingData FromJson(const nlohmann::json& j);
	};

	// Item data parsed from JSON
	struct ItemData {
		TargetingData targeting;
		std::vector<EffectData> effects;

		static ItemData FromJson(const nlohmann::json& j);
	};

	// Complete entity definition from JSON
	struct EntityTemplate {
		std::string id;   // Template ID (e.g., "orc", "health_potion")
		std::string name; // Display name
		char icon;        // Character to render
		tcod::ColorRGB color; // RGB color
		bool blocks;          // Blocks movement?
		std::string faction;  // "monster", "player", "neutral"

		// Combat stats
		int hp;
		int maxHp;
		int defense;
		int power;
		int xpReward;

		// AI type
		std::optional<std::string>
		    aiType; // "hostile", "player", "confused"

		// Optional item data
		std::optional<ItemData> item;

		// Spawn locations (where/when this entity appears)
		std::vector<SpawnData> spawns;

		// Can this item be picked up? (defaults to true for items,
		// false for corpses)
		bool pickable = true; // ADD THIS LINE

		// Parse from JSON object
		static EntityTemplate FromJson(const std::string& id,
		                               const nlohmann::json& j);

		// Convert back to JSON (for saves later)
		nlohmann::json ToJson() const;

		// Factory method: create Entity from this template
		std::unique_ptr<Entity> CreateEntity(pos_t pos) const;
	};
} // namespace tutorial

#endif // ENTITY_TEMPLATE_HPP
