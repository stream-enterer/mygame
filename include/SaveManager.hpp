#ifndef SAVE_MANAGER_HPP
#define SAVE_MANAGER_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace tutorial
{
	// Forward declarations
	class Engine;
	class Entity;
	class AiComponent;
	struct LevelConfig;
	struct AttackerComponent;
	struct DestructibleComponent;
	struct IconRenderable;
	struct pos_t;
	enum class Faction;

	enum class SaveType {
		Manual, // Save-on-quit
		Auto    // Autosave (periodic)
	};

	class SaveManager
	{
	public:
		// Get singleton instance
		static SaveManager& Instance();

		// Save/Load operations
		bool SaveGame(const Engine& engine,
		              SaveType type = SaveType::Manual);
		bool LoadGame(Engine& engine);

		// Check if save exists
		bool HasSave() const;

		// Delete save (on game over)
		void DeleteSave();

		// Get save file path
		std::string GetSavePath() const;

		// Get metadata about save (for UI display)
		struct SaveMetadata {
			std::string playerName;
			int playerLevel;
			int playerHP;
			int playerMaxHP;
			std::string levelName;
			std::string timestamp;
			bool valid;
		};
		SaveMetadata GetSaveMetadata() const;

		std::unique_ptr<Entity> DeserializeEntity(
		    const nlohmann::json& j);

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

		nlohmann::json SerializeMap(const Engine& engine) const;
		bool DeserializeMap(const nlohmann::json& j, Engine& engine);

		// File operations
		bool WriteToFile(const nlohmann::json& j) const;
		nlohmann::json ReadFromFile() const;

		std::string GetTimestamp() const;

		static bool LoadTemplatesAndSpawnTables(
		    const LevelConfig& config);
		static bool InitializeEngineState(Engine& engine,
		                                  const nlohmann::json& j);
		static bool RestorePlayerAndUI(
		    Engine& engine, const nlohmann::json& playerJson);
		static void RegenerateEntitiesAndStairs(
		    Engine& engine, const LevelConfig& config);
		static AttackerComponent ParseAttackerComponent(
		    const nlohmann::json& j);
		static DestructibleComponent ParseDestructibleComponent(
		    const nlohmann::json& j);
		static IconRenderable ParseRenderableComponent(
		    const nlohmann::json& j);
		static std::unique_ptr<AiComponent> ParseAiComponent(
		    const nlohmann::json& j);
		static std::unique_ptr<Entity> CreatePlayerEntity(
		    const nlohmann::json& j, pos_t pos, const std::string& name,
		    const std::string& pluralName, int stackCount,
		    const std::string& templateId, int renderPriority,
		    bool blocker, AttackerComponent attacker,
		    DestructibleComponent destructible,
		    IconRenderable renderable, Faction faction, bool pickable,
		    bool isCorpse);
		static std::unique_ptr<Entity> CreateNpcEntity(
		    const nlohmann::json& j, pos_t pos, const std::string& name,
		    const std::string& pluralName, int stackCount,
		    const std::string& templateId, int renderPriority,
		    bool blocker, AttackerComponent attacker,
		    DestructibleComponent destructible,
		    IconRenderable renderable, Faction faction, bool pickable,
		    bool isCorpse);
		static std::unique_ptr<Entity> RestoreItemFromTemplate(
		    const nlohmann::json& j, pos_t pos, const std::string& name,
		    const std::string& pluralName, int stackCount,
		    const std::string& templateId, int renderPriority);
		static std::unique_ptr<Entity> CreateItemEntity(
		    const nlohmann::json& j, pos_t pos, const std::string& name,
		    const std::string& pluralName, int stackCount,
		    const std::string& templateId, int renderPriority,
		    bool blocker, AttackerComponent attacker,
		    DestructibleComponent destructible,
		    IconRenderable renderable, Faction faction, bool pickable,
		    bool isCorpse);
		static void ExtractPlayerMetadata(
		    const nlohmann::json& engineData, SaveMetadata& metadata);
		static void ExtractLevelMetadata(
		    const nlohmann::json& engineData, SaveMetadata& metadata);

		const std::string kSaveFileName = "save.json";
		const std::string kSaveDirectory = "data/saves/";
	};

} // namespace tutorial

#endif // SAVE_MANAGER_HPP
