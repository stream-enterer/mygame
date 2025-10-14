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

    // Item data parsed from JSON
    struct ItemData
    {
        std::string type; // "heal", "lightning", "fireball", "confuse"

        // Optional fields depending on item type
        std::optional<int> amount;   // For heal items
        std::optional<float> range;  // For targeted items
        std::optional<float> damage; // For damage items
        std::optional<int> turns;    // For confuse items

        static ItemData FromJson(const nlohmann::json& j);
    };

    // Complete entity definition from JSON
    struct EntityTemplate
    {
        std::string id;       // Template ID (e.g., "orc", "health_potion")
        std::string name;     // Display name
        char icon;            // Character to render
        tcod::ColorRGB color; // RGB color
        bool blocks;          // Blocks movement?
        std::string faction;  // "monster", "player", "neutral"

        // Combat stats
        int hp;
        int maxHp;
        int defense;
        int power;

        // AI type
        std::string aiType; // "hostile", "player", "confused"

        // Optional item data
        std::optional<ItemData> item;

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
