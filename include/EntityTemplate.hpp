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

	struct ItemTemplate {
		std::string id;   // Filename becomes ID (e.g., "health_potion")
		std::string name; // Display name
		char icon;        // Character to render
		tcod::ColorRGB color; // RGB color

		// Targeting - flat structure
		std::string targetingType; // "self", "closest_enemy", "single",
		                           // "area", "beam"
		std::optional<float> range;
		std::optional<float> radius;

		// Effects stored as JSON, parsed when creating Item component
		nlohmann::json effects;

		// Parse from JSON (expects flattened item structure)
		static ItemTemplate FromJson(const std::string& id,
		                             const nlohmann::json& j);

		// Create entity with item component
		std::unique_ptr<Entity> CreateEntity(pos_t pos) const;
	};

	struct UnitTemplate {
		std::string id;       // Filename becomes ID (e.g., "orc")
		std::string name;     // Display name
		char icon;            // Character to render
		tcod::ColorRGB color; // RGB color
		bool blocks;          // Blocks movement?

		// Combat stats
		int hp;
		int defense;
		int power;
		int xp; // XP reward when killed

		// AI type (e.g., "hostile")
		std::string ai;

		// Parse from JSON (expects flattened unit structure)
		static UnitTemplate FromJson(const std::string& id,
		                             const nlohmann::json& j);

		// Create entity (monster with AI)
		std::unique_ptr<Entity> CreateEntity(pos_t pos) const;
	};

	// Effect data parsed from JSON (used internally by
	// LoadItemsFromDirectory)
	struct EffectData {
		std::string type;
		std::optional<int> amount;
		std::optional<std::string> aiType;
		std::optional<int> duration;
		std::optional<std::string> messageKey;
	};

	// Target selector data parsed from JSON (used internally by
	// LoadItemsFromDirectory)
	struct TargetingData {
		std::string type;
		std::optional<float> range;
		std::optional<float> radius;
	};

	// Item data parsed from JSON (used internally by
	// LoadItemsFromDirectory)
	struct ItemData {
		TargetingData targeting;
		std::vector<EffectData> effects;
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
