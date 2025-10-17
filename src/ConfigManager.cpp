#include "ConfigManager.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace tutorial
{
    ConfigManager& ConfigManager::Instance()
    {
        static ConfigManager instance;
        return instance;
    }

    void ConfigManager::LoadAll()
    {
        std::cout << "[ConfigManager] Loading configuration files..."
                  << std::endl;

        try
        {
            LoadGameConfig();
            LoadUIConfig();

            std::cout
                << "[ConfigManager] All configuration files loaded successfully"
                << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ConfigManager] FATAL: Failed to load configuration: "
                      << e.what() << std::endl;
            throw; // Re-throw to crash at startup
        }
    }

    void ConfigManager::Clear()
    {
        gameConfig_.clear();
        uiConfig_.clear();
    }

    void ConfigManager::LoadGameConfig()
    {
        const std::string filepath = "data/config/game.json";
        std::ifstream file(filepath);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open game config: " + filepath);
        }

        try
        {
            file >> gameConfig_;
            std::cout << "[ConfigManager] Loaded: " << filepath << std::endl;
        }
        catch (const nlohmann::json::parse_error& e)
        {
            throw std::runtime_error("JSON parse error in " + filepath + ": "
                                     + e.what());
        }

        // Validate required fields
        if (!gameConfig_.contains("player"))
        {
            throw std::runtime_error(
                "game.json missing required 'player' section");
        }
    }

    void ConfigManager::LoadUIConfig()
    {
        const std::string filepath = "data/config/ui.json";
        std::ifstream file(filepath);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open UI config: " + filepath);
        }

        try
        {
            file >> uiConfig_;
            std::cout << "[ConfigManager] Loaded: " << filepath << std::endl;
        }
        catch (const nlohmann::json::parse_error& e)
        {
            throw std::runtime_error("JSON parse error in " + filepath + ": "
                                     + e.what());
        }

        // Validate required fields
        if (!uiConfig_.contains("layout"))
        {
            throw std::runtime_error(
                "ui.json missing required 'layout' section");
        }
    }

    // === Game Config Accessors ===

    int ConfigManager::GetPlayerFOVRadius() const
    {
        if (!gameConfig_.contains("player")
            || !gameConfig_["player"].contains("fov_radius"))
        {
            throw std::runtime_error("game.json missing player.fov_radius");
        }
        return gameConfig_["player"]["fov_radius"].get<int>();
    }

    int ConfigManager::GetMaxInventorySize() const
    {
        if (!gameConfig_.contains("player")
            || !gameConfig_["player"].contains("max_inventory_size"))
        {
            throw std::runtime_error(
                "game.json missing player.max_inventory_size");
        }
        return gameConfig_["player"]["max_inventory_size"].get<int>();
    }

    float ConfigManager::GetDifficultyMultiplier() const
    {
        if (!gameConfig_.contains("difficulty"))
        {
            return 1.0f; // Default
        }
        return gameConfig_["difficulty"].value("damage_multiplier", 1.0f);
    }

    bool ConfigManager::IsDebugShowAllMap() const
    {
        if (!gameConfig_.contains("debug"))
        {
            return false;
        }
        return gameConfig_["debug"].value("show_all_map", false);
    }

    bool ConfigManager::IsDebugInvincible() const
    {
        if (!gameConfig_.contains("debug"))
        {
            return false;
        }
        return gameConfig_["debug"].value("invincible_player", false);
    }

    bool ConfigManager::IsDebugLogAI() const
    {
        if (!gameConfig_.contains("debug"))
        {
            return false;
        }
        return gameConfig_["debug"].value("log_ai_decisions", false);
    }

    // === UI Config Accessors ===

    int ConfigManager::GetMapHeightOffset() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("map_height_offset"))
        {
            throw std::runtime_error(
                "ui.json missing layout.map_height_offset");
        }
        return uiConfig_["layout"]["map_height_offset"].get<int>();
    }

    int ConfigManager::GetHealthBarWidth() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("health_bar"))
        {
            throw std::runtime_error("ui.json missing layout.health_bar");
        }
        return uiConfig_["layout"]["health_bar"]["width"].get<int>();
    }

    int ConfigManager::GetHealthBarHeight() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("health_bar"))
        {
            throw std::runtime_error("ui.json missing layout.health_bar");
        }
        return uiConfig_["layout"]["health_bar"]["height"].get<int>();
    }

    int ConfigManager::GetHealthBarX() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("health_bar")
            || !uiConfig_["layout"]["health_bar"].contains("position"))
        {
            throw std::runtime_error(
                "ui.json missing layout.health_bar.position");
        }
        return uiConfig_["layout"]["health_bar"]["position"]["x"].get<int>();
    }

    int ConfigManager::GetHealthBarY() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("health_bar")
            || !uiConfig_["layout"]["health_bar"].contains("position"))
        {
            throw std::runtime_error(
                "ui.json missing layout.health_bar.position");
        }
        return uiConfig_["layout"]["health_bar"]["position"]["y"].get<int>();
    }

    tcod::ColorRGB ConfigManager::GetHealthBarFullColor() const
    {
        if (!uiConfig_.contains("colors")
            || !uiConfig_["colors"].contains("health_bar_full"))
        {
            throw std::runtime_error("ui.json missing colors.health_bar_full");
        }
        auto colorArray = uiConfig_["colors"]["health_bar_full"];
        return tcod::ColorRGB{ static_cast<uint8_t>(colorArray[0].get<int>()),
                               static_cast<uint8_t>(colorArray[1].get<int>()),
                               static_cast<uint8_t>(colorArray[2].get<int>()) };
    }

    tcod::ColorRGB ConfigManager::GetHealthBarEmptyColor() const
    {
        if (!uiConfig_.contains("colors")
            || !uiConfig_["colors"].contains("health_bar_empty"))
        {
            throw std::runtime_error("ui.json missing colors.health_bar_empty");
        }
        auto colorArray = uiConfig_["colors"]["health_bar_empty"];
        return tcod::ColorRGB{ static_cast<uint8_t>(colorArray[0].get<int>()),
                               static_cast<uint8_t>(colorArray[1].get<int>()),
                               static_cast<uint8_t>(colorArray[2].get<int>()) };
    }

    int ConfigManager::GetMessageLogWidth() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("message_log"))
        {
            throw std::runtime_error("ui.json missing layout.message_log");
        }
        return uiConfig_["layout"]["message_log"]["width"].get<int>();
    }

    int ConfigManager::GetMessageLogHeight() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("message_log"))
        {
            throw std::runtime_error("ui.json missing layout.message_log");
        }
        return uiConfig_["layout"]["message_log"]["height"].get<int>();
    }

    int ConfigManager::GetMessageLogX() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("message_log")
            || !uiConfig_["layout"]["message_log"].contains("position"))
        {
            throw std::runtime_error(
                "ui.json missing layout.message_log.position");
        }
        return uiConfig_["layout"]["message_log"]["position"]["x"].get<int>();
    }

    int ConfigManager::GetMessageLogY() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("message_log")
            || !uiConfig_["layout"]["message_log"].contains("position"))
        {
            throw std::runtime_error(
                "ui.json missing layout.message_log.position");
        }
        return uiConfig_["layout"]["message_log"]["position"]["y"].get<int>();
    }

    int ConfigManager::GetInventoryWindowWidth() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("inventory_window"))
        {
            throw std::runtime_error("ui.json missing layout.inventory_window");
        }
        return uiConfig_["layout"]["inventory_window"]["width"].get<int>();
    }

    int ConfigManager::GetInventoryWindowHeight() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("inventory_window"))
        {
            throw std::runtime_error("ui.json missing layout.inventory_window");
        }
        return uiConfig_["layout"]["inventory_window"]["height"].get<int>();
    }

    bool ConfigManager::GetInventoryCenterOnScreen() const
    {
        if (!uiConfig_.contains("layout")
            || !uiConfig_["layout"].contains("inventory_window"))
        {
            throw std::runtime_error("ui.json missing layout.inventory_window");
        }
        return uiConfig_["layout"]["inventory_window"]["center_on_screen"]
            .get<bool>();
    }

    tcod::ColorRGB ConfigManager::GetUIFrameColor() const
    {
        if (!uiConfig_.contains("colors")
            || !uiConfig_["colors"].contains("ui_frame"))
        {
            throw std::runtime_error("ui.json missing colors.ui_frame");
        }
        auto colorArray = uiConfig_["colors"]["ui_frame"];
        return tcod::ColorRGB{ static_cast<uint8_t>(colorArray[0].get<int>()),
                               static_cast<uint8_t>(colorArray[1].get<int>()),
                               static_cast<uint8_t>(colorArray[2].get<int>()) };
    }

    tcod::ColorRGB ConfigManager::GetUITextColor() const
    {
        if (!uiConfig_.contains("colors")
            || !uiConfig_["colors"].contains("ui_text"))
        {
            throw std::runtime_error("ui.json missing colors.ui_text");
        }
        auto colorArray = uiConfig_["colors"]["ui_text"];
        return tcod::ColorRGB{ static_cast<uint8_t>(colorArray[0].get<int>()),
                               static_cast<uint8_t>(colorArray[1].get<int>()),
                               static_cast<uint8_t>(colorArray[2].get<int>()) };
    }

} // namespace tutorial
