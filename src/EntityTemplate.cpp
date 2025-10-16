#include "EntityTemplate.hpp"

#include "AiComponent.hpp"
#include "Colors.hpp"
#include "Components.hpp"
#include "Entity.hpp"
#include "Item.hpp"
#include "Position.hpp"

#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace tutorial
{

    SpawnData SpawnData::FromJson(const json& j)
    {
        SpawnData data;

        if (!j.contains("location"))
        {
            throw std::runtime_error(
                "Spawn data missing required 'location' field");
        }
        data.location = j["location"];

        if (!j.contains("weight"))
        {
            throw std::runtime_error(
                "Spawn data missing required 'weight' field");
        }
        data.weight = j["weight"];

        return data;
    }

    ItemData ItemData::FromJson(const json& j)
    {
        ItemData data;

        // Type is required
        if (!j.contains("type"))
        {
            throw std::runtime_error("Item data missing required 'type' field");
        }
        data.type = j["type"];

        // Optional fields based on item type
        if (j.contains("amount"))
        {
            data.amount = j["amount"];
        }
        if (j.contains("range"))
        {
            data.range = j["range"];
        }
        if (j.contains("damage"))
        {
            data.damage = j["damage"];
        }
        if (j.contains("turns"))
        {
            data.turns = j["turns"];
        }

        return data;
    }

    EntityTemplate EntityTemplate::FromJson(const std::string& id,
                                            const json& j)
    {
        EntityTemplate tpl;

        tpl.id = id;

        // Required fields - throw if missing
        if (!j.contains("name"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'name'");
        }
        tpl.name = j["name"];

        if (!j.contains("char"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'char'");
        }
        std::string charStr = j["char"];
        if (charStr.empty())
        {
            throw std::runtime_error("Entity '" + id
                                     + "' has empty 'char' field");
        }
        tpl.icon = charStr[0];

        if (!j.contains("color") || !j["color"].is_array()
            || j["color"].size() != 3)
        {
            throw std::runtime_error(
                "Entity '" + id
                + "' missing or invalid 'color' field (must be [r,g,b])");
        }
        tpl.color =
            tcod::ColorRGB{ static_cast<uint8_t>(j["color"][0].get<int>()),
                            static_cast<uint8_t>(j["color"][1].get<int>()),
                            static_cast<uint8_t>(j["color"][2].get<int>()) };

        if (!j.contains("blocks"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'blocks'");
        }
        tpl.blocks = j["blocks"];

        if (!j.contains("faction"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'faction'");
        }
        tpl.faction = j["faction"];

        if (!j.contains("hp"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'hp'");
        }
        tpl.hp = j["hp"];

        if (!j.contains("maxHp"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'maxHp'");
        }
        tpl.maxHp = j["maxHp"];

        if (!j.contains("defense"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'defense'");
        }
        tpl.defense = j["defense"];

        if (!j.contains("power"))
        {
            throw std::runtime_error("Entity '" + id
                                     + "' missing required field 'power'");
        }
        tpl.power = j["power"];

        // AI is optional (items don't need it)
        if (j.contains("ai"))
        {
            tpl.aiType = j["ai"];
        }

        // Optional item data
        if (j.contains("item"))
        {
            tpl.item = ItemData::FromJson(j["item"]);
        }

        // Optional spawn data
        if (j.contains("spawns") && j["spawns"].is_array())
        {
            for (const auto& spawnJson : j["spawns"])
            {
                try
                {
                    tpl.spawns.push_back(SpawnData::FromJson(spawnJson));
                }
                catch (const std::exception& e)
                {
                    std::cerr
                        << "[EntityTemplate] Warning: Invalid spawn data for '"
                        << id << "': " << e.what() << std::endl;
                }
            }
        }

        // Validation: Monsters must have AI
        if (tpl.faction == "monster" && !tpl.aiType.has_value())
        {
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
        if (aiType.has_value())
        {
            j["ai"] = aiType.value();
        }

        if (item.has_value())
        {
            json itemJson;
            itemJson["type"] = item->type;
            if (item->amount.has_value())
            {
                itemJson["amount"] = item->amount.value();
            }
            if (item->range.has_value())
            {
                itemJson["range"] = item->range.value();
            }
            if (item->damage.has_value())
            {
                itemJson["damage"] = item->damage.value();
            }
            if (item->turns.has_value())
            {
                itemJson["turns"] = item->turns.value();
            }
            j["item"] = itemJson;
        }

        // Serialize spawn data
        if (!spawns.empty())
        {
            json spawnsArray = json::array();
            for (const auto& spawn : spawns)
            {
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
        if (faction == "player")
        {
            factionEnum = Faction::PLAYER;
        }
        else if (faction == "monster")
        {
            factionEnum = Faction::MONSTER;
        }
        else
        {
            factionEnum = Faction::NEUTRAL;
        }

        // Create item component if this template has item data
        std::unique_ptr<Item> itemComponent = nullptr;
        if (item.has_value())
        {
            const auto& itemData = item.value();

            if (itemData.type == "heal")
            {
                itemComponent =
                    std::make_unique<HealthPotion>(itemData.amount.value_or(4));
            }
            else if (itemData.type == "lightning")
            {
                itemComponent = std::make_unique<LightningBolt>(
                    itemData.range.value_or(5.0f),
                    itemData.damage.value_or(20.0f));
            }
            else if (itemData.type == "fireball")
            {
                itemComponent =
                    std::make_unique<Fireball>(itemData.range.value_or(3.0f),
                                               itemData.damage.value_or(12.0f));
            }
            else if (itemData.type == "confuse")
            {
                itemComponent = std::make_unique<Confuser>(
                    itemData.turns.value_or(10), itemData.range.value_or(8.0f));
            }
        }

        // Create AI component based on type (if AI type is specified)
        std::unique_ptr<AiComponent> aiComponent = nullptr;
        if (aiType.has_value())
        {
            if (aiType.value() == "hostile")
            {
                aiComponent = std::make_unique<HostileAi>();
            }
            else if (aiType.value() == "player")
            {
                // Player AI will be set by Player class itself
                aiComponent = nullptr;
            }
        }

        // Create appropriate entity type
        if (factionEnum == Faction::PLAYER)
        {
            return std::make_unique<Player>(
                pos, name, blocks,
                AttackerComponent{ static_cast<unsigned int>(power) },
                DestructibleComponent{ static_cast<unsigned int>(defense),
                                       static_cast<unsigned int>(maxHp),
                                       static_cast<unsigned int>(hp) },
                IconRenderable{ color, icon }, factionEnum);
        }
        else if (aiComponent != nullptr)
        {
            // Monster with AI
            return std::make_unique<Npc>(
                pos, name, blocks,
                AttackerComponent{ static_cast<unsigned int>(power) },
                DestructibleComponent{ static_cast<unsigned int>(defense),
                                       static_cast<unsigned int>(maxHp),
                                       static_cast<unsigned int>(hp) },
                IconRenderable{ color, icon }, factionEnum,
                std::move(aiComponent));
        }
        else
        {
            // Item or neutral entity without AI
            return std::make_unique<BaseEntity>(
                pos, name, blocks,
                AttackerComponent{ static_cast<unsigned int>(power) },
                DestructibleComponent{ static_cast<unsigned int>(defense),
                                       static_cast<unsigned int>(maxHp),
                                       static_cast<unsigned int>(hp) },
                IconRenderable{ color, icon }, factionEnum,
                std::move(itemComponent));
        }
    }
} // namespace tutorial
