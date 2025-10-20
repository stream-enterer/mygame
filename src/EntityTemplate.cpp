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

    EffectData EffectData::FromJson(const json& j)
    {
        EffectData data;

        if (!j.contains("type"))
        {
            throw std::runtime_error("Effect missing required 'type' field");
        }
        data.type = j["type"];

        if (j.contains("amount"))
        {
            data.amount = j["amount"];
        }
        if (j.contains("aiType"))
        {
            data.aiType = j["aiType"];
        }
        if (j.contains("duration"))
        {
            data.duration = j["duration"];
        }
        if (j.contains("messageKey"))
        {
            data.messageKey = j["messageKey"];
        }

        return data;
    }

    TargetingData TargetingData::FromJson(const json& j)
    {
        TargetingData data;

        if (!j.contains("type"))
        {
            throw std::runtime_error("Targeting missing required 'type' field");
        }
        data.type = j["type"];

        if (j.contains("range"))
        {
            data.range = j["range"];
        }
        if (j.contains("radius"))
        {
            data.radius = j["radius"];
        }

        return data;
    }

    ItemData ItemData::FromJson(const json& j)
    {
        ItemData data;

        if (!j.contains("targeting"))
        {
            throw std::runtime_error("Item missing required 'targeting' field");
        }
        data.targeting = TargetingData::FromJson(j["targeting"]);

        if (!j.contains("effects") || !j["effects"].is_array())
        {
            throw std::runtime_error(
                "Item missing required 'effects' array field");
        }

        for (const auto& effectJson : j["effects"])
        {
            data.effects.push_back(EffectData::FromJson(effectJson));
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

        // Parse xpReward (optional, defaults to 0)
        tpl.xpReward = j.value("xpReward", 0);

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

        // Pickable flag (defaults to true if not specified)
        if (j.contains("pickable"))
        {
            tpl.pickable = j["pickable"];
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

            // Serialize targeting
            json targetingJson;
            targetingJson["type"] = item->targeting.type;
            if (item->targeting.range.has_value())
            {
                targetingJson["range"] = item->targeting.range.value();
            }
            if (item->targeting.radius.has_value())
            {
                targetingJson["radius"] = item->targeting.radius.value();
            }
            itemJson["targeting"] = targetingJson;

            // Serialize effects array
            json effectsArray = json::array();
            for (const auto& effect : item->effects)
            {
                json effectJson;
                effectJson["type"] = effect.type;
                if (effect.amount.has_value())
                {
                    effectJson["amount"] = effect.amount.value();
                }
                if (effect.aiType.has_value())
                {
                    effectJson["aiType"] = effect.aiType.value();
                }
                if (effect.duration.has_value())
                {
                    effectJson["duration"] = effect.duration.value();
                }
                if (effect.messageKey.has_value())
                {
                    effectJson["messageKey"] = effect.messageKey.value();
                }
                effectsArray.push_back(effectJson);
            }
            itemJson["effects"] = effectsArray;

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

            // Create target selector
            std::unique_ptr<TargetSelector> selector;
            const auto& targeting = itemData.targeting;

            if (targeting.type == "self")
            {
                selector = std::make_unique<SelfTargetSelector>();
            }
            else if (targeting.type == "closest_enemy")
            {
                float range = targeting.range.value_or(5.0f);
                selector = std::make_unique<ClosestEnemySelector>(range);
            }
            else if (targeting.type == "single")
            {
                float range = targeting.range.value_or(8.0f);
                selector = std::make_unique<SingleTargetSelector>(range);
            }
            else if (targeting.type == "area")
            {
                float range = targeting.range.value_or(3.0f);
                float radius = targeting.radius.value_or(3.0f);
                selector = std::make_unique<AreaTargetSelector>(range, radius);
            }
            else
            {
                throw std::runtime_error("Unknown targeting type: "
                                         + targeting.type);
            }

            // Create effects
            std::vector<std::unique_ptr<Effect>> effects;
            for (const auto& effectData : itemData.effects)
            {
                if (effectData.type == "health")
                {
                    int amount = effectData.amount.value_or(0);
                    std::string msgKey =
                        effectData.messageKey.value_or(std::string());
                    effects.push_back(
                        std::make_unique<HealthEffect>(amount, msgKey));
                }
                else if (effectData.type == "ai_change")
                {
                    std::string aiType = effectData.aiType.value_or("confused");
                    int duration = effectData.duration.value_or(10);
                    std::string msgKey =
                        effectData.messageKey.value_or(std::string());
                    effects.push_back(std::make_unique<AiChangeEffect>(
                        aiType, duration, msgKey));
                }
                else
                {
                    throw std::runtime_error("Unknown effect type: "
                                             + effectData.type);
                }
            }

            itemComponent =
                std::make_unique<Item>(std::move(selector), std::move(effects));
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
                IconRenderable{ color, icon }, factionEnum, pickable);
        }
        else if (aiComponent != nullptr)
        {
            // Monster with AI - create destructible with XP reward
            DestructibleComponent destructible{
                static_cast<unsigned int>(defense),
                static_cast<unsigned int>(maxHp), static_cast<unsigned int>(hp)
            };
            destructible.SetXpReward(static_cast<unsigned int>(xpReward));

            return std::make_unique<Npc>(
                pos, name, blocks,
                AttackerComponent{ static_cast<unsigned int>(power) },
                destructible, IconRenderable{ color, icon }, factionEnum,
                std::move(aiComponent), pickable);
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
                std::move(itemComponent), pickable);
        }
    }
} // namespace tutorial
