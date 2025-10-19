#ifndef SAVE_MANAGER_HPP
#define SAVE_MANAGER_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace tutorial
{
    class Engine;
    class Entity;

    enum class SaveType
    {
        Manual, // Save-on-quit
        Auto    // Autosave (periodic)
    };

    class SaveManager
    {
    public:
        // Get singleton instance
        static SaveManager& Instance();

        // Save/Load operations
        bool SaveGame(const Engine& engine, SaveType type = SaveType::Manual);
        bool LoadGame(Engine& engine);

        // Check if save exists
        bool HasSave() const;

        // Delete save (on game over)
        void DeleteSave();

        // Get save file path
        std::string GetSavePath() const;

        // Get metadata about save (for UI display)
        struct SaveMetadata
        {
            std::string playerName;
            int playerLevel;
            int playerHP;
            int playerMaxHP;
            std::string levelName;
            std::string timestamp;
            bool valid;
        };
        SaveMetadata GetSaveMetadata() const;

    private:
        SaveManager() = default;
        ~SaveManager() = default;

        // Prevent copying
        SaveManager(const SaveManager&) = delete;
        SaveManager& operator=(const SaveManager&) = delete;

        // Serialization helpers
        nlohmann::json SerializeEngine(const Engine& engine) const;
        bool DeserializeEngine(const nlohmann::json& j, Engine& engine);

        nlohmann::json SerializeEntity(const Entity& entity) const;
        std::unique_ptr<Entity> DeserializeEntity(const nlohmann::json& j);

        nlohmann::json SerializeMap(const Engine& engine) const;
        bool DeserializeMap(const nlohmann::json& j, Engine& engine);

        // File operations
        bool WriteToFile(const nlohmann::json& j) const;
        nlohmann::json ReadFromFile() const;

        std::string GetTimestamp() const;

        const std::string kSaveFileName = "save.json";
        const std::string kSaveDirectory = "data/saves/";
    };

} // namespace tutorial

#endif // SAVE_MANAGER_HPP
