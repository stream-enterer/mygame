#include "SaveManager.hpp"

#include "AiComponent.hpp"
#include "Components.hpp"
#include "ConfigManager.hpp"
#include "DynamicSpawnSystem.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "EventHandler.hpp"
#include "HealthBar.hpp"
#include "InventoryWindow.hpp"
#include "LevelConfig.hpp"
#include "LocaleManager.hpp"
#include "Map.hpp"
#include "TemplateRegistry.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace tutorial
{
	// Helper functions to reduce nesting (now static members)
	bool SaveManager::LoadTemplatesAndSpawnTables(const LevelConfig& config)
	{
		try {
			TemplateRegistry::Instance().Clear();
			TemplateRegistry::Instance().LoadFromDirectory(
			    "data/entities");
			TemplateRegistry::Instance().LoadSimplifiedDirectory(
			    "data/units", "unit");
			TemplateRegistry::Instance().LoadSimplifiedDirectory(
			    "data/items", "item");

			std::cout
			    << "[SaveManager] Loaded "
			    << TemplateRegistry::Instance().GetAllIds().size()
			    << " templates (special + units + items)"
			    << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "[SaveManager] FATAL: Failed to "
			             "load entity templates: "
			          << e.what() << std::endl;
			throw;
		}

		try {
			DynamicSpawnSystem::Instance().Clear();
			DynamicSpawnSystem::Instance().BuildSpawnTablesForLevel(
			    config);
		} catch (const std::exception& e) {
			std::cerr << "[SaveManager] FATAL: Failed to "
			             "build spawn tables: "
			          << e.what() << std::endl;
			throw;
		}

		return true;
	}

	bool SaveManager::InitializeEngineState(Engine& engine,
	                                        const nlohmann::json& j)
	{
		// Clear existing state
		engine.entities_.Clear();
		engine.messageLog_.Clear();
		engine.eventQueue_.clear();
		engine.entitiesToRemove_.clear();

		// Restore dungeon level
		if (j["level"].contains("dungeonLevel")) {
			engine.dungeonLevel_ =
			    j["level"]["dungeonLevel"].get<int>();
			std::cout << "[SaveManager] Restored dungeon level: "
			          << engine.dungeonLevel_ << std::endl;
		} else {
			engine.dungeonLevel_ = 1;
		}

		return true;
	}

	bool SaveManager::RestorePlayerAndUI(Engine& engine,
	                                     const nlohmann::json& playerJson)
	{
		auto playerEntity =
		    SaveManager::Instance().DeserializeEntity(playerJson);
		if (!playerEntity) {
			std::cerr << "[SaveManager] Failed to "
			             "restore player"
			          << std::endl;
			return false;
		}

		// Place player at first room center
		if (engine.map_->GetRooms().empty()) {
			std::cerr << "[SaveManager] No rooms "
			             "generated in map"
			          << std::endl;
			return false;
		}

		pos_t safePos = engine.map_->GetRooms()[0].GetCenter();
		playerEntity->SetPos(safePos);

		engine.player_ =
		    engine.entities_.Spawn(std::move(playerEntity)).get();

		std::cout << "[SaveManager] Player restored at ("
		          << engine.player_->GetPos().x << ", "
		          << engine.player_->GetPos().y << ") "
		          << "(placed at first room center)" << std::endl;

		// Create UI components
		auto& cfg = ConfigManager::Instance();

		engine.healthBar_ = std::make_unique<HealthBar>(
		    cfg.GetHealthBarWidth(), cfg.GetHealthBarHeight(),
		    pos_t { cfg.GetHealthBarX(), cfg.GetHealthBarY() },
		    *engine.player_);

		int invWidth = cfg.GetInventoryWindowWidth();
		int invHeight = cfg.GetInventoryWindowHeight();
		pos_t invPos;

		if (cfg.GetInventoryCenterOnScreen()) {
			invPos =
			    pos_t { static_cast<int>(engine.config_.width) / 2
				        - invWidth / 2,
				    static_cast<int>(engine.config_.height) / 2
				        - invHeight / 2 };
		} else {
			invPos = pos_t { 0, 0 };
		}

		engine.inventoryWindow_ = std::make_unique<InventoryWindow>(
		    invWidth, invHeight, invPos, *engine.player_);

		return true;
	}

	void SaveManager::RegenerateEntitiesAndStairs(Engine& engine,
	                                              const LevelConfig& config)
	{
		std::cout << "[SaveManager] Regenerating monsters and "
		             "items..."
		          << std::endl;

		const auto& rooms = engine.map_->GetRooms();
		for (size_t i = 1; i < rooms.size(); ++i) {
			engine.entities_.PlaceEntities(
			    rooms[i], config.monsterSpawning, config.id);
			engine.entities_.PlaceItems(
			    rooms[i], config.itemSpawning, config.id);
		}

		// Place stairs in last room
		if (!rooms.empty()) {
			pos_t stairsPos = rooms.back().GetCenter();
			auto stairsEntity = TemplateRegistry::Instance().Create(
			    "stairs_down", stairsPos);
			engine.stairs_ =
			    engine.entities_.Spawn(std::move(stairsEntity))
			        .get();
			std::cout << "[SaveManager] Placed stairs at ("
			          << stairsPos.x << ", " << stairsPos.y << ")"
			          << std::endl;
		}
	}

	AttackerComponent SaveManager::ParseAttackerComponent(
	    const nlohmann::json& j)
	{
		if (!j.contains("attacker")) {
			return AttackerComponent { 1 };
		}

		unsigned int strength = 1;
		if (j["attacker"].contains("strength")) {
			strength =
			    j["attacker"]["strength"].get<unsigned int>();
		} else if (j["attacker"].contains("power")) {
			strength = j["attacker"]["power"].get<unsigned int>();
		}

		return AttackerComponent { strength };
	}

	DestructibleComponent SaveManager::ParseDestructibleComponent(
	    const nlohmann::json& j)
	{
		if (!j.contains("destructible")) {
			return DestructibleComponent { 1, 1, 1 };
		}

		unsigned int dexterity = 1;
		if (j["destructible"].contains("dexterity")) {
			dexterity =
			    j["destructible"]["dexterity"].get<unsigned int>();
		} else if (j["destructible"].contains("defense")) {
			dexterity =
			    j["destructible"]["defense"].get<unsigned int>();
		}

		unsigned int maxHp =
		    j["destructible"]["maxHp"].get<unsigned int>();
		unsigned int hp = j["destructible"]["hp"].get<unsigned int>();

		DestructibleComponent destructible { dexterity, maxHp, hp };

		// Restore XP data
		if (j["destructible"].contains("xp")) {
			unsigned int xp =
			    j["destructible"]["xp"].get<unsigned int>();
			destructible.AddXp(xp);
		}
		if (j["destructible"].contains("xpReward")) {
			unsigned int xpReward =
			    j["destructible"]["xpReward"].get<unsigned int>();
			destructible.SetXpReward(xpReward);
		}

		// Restore mana/INT data
		if (j["destructible"].contains("intelligence")) {
			unsigned int intelligence =
			    j["destructible"]["intelligence"]
			        .get<unsigned int>();
			if (intelligence > 1) {
				destructible.IncreaseIntelligence(intelligence
				                                  - 1);
			}
		}
		if (j["destructible"].contains("mp")) {
			unsigned int mp =
			    j["destructible"]["mp"].get<unsigned int>();
			unsigned int currentMp = destructible.GetMp();
			if (mp < currentMp) {
				destructible.SpendMp(currentMp - mp);
			} else if (mp > currentMp) {
				destructible.RegenerateMp(mp - currentMp);
			}
		}

		return destructible;
	}

	IconRenderable SaveManager::ParseRenderableComponent(
	    const nlohmann::json& j)
	{
		char icon = '@';
		tcod::ColorRGB color { 255, 255, 255 };

		if (!j.contains("renderable")) {
			return IconRenderable { color, icon };
		}

		std::string iconStr =
		    j["renderable"]["icon"].get<std::string>();
		if (!iconStr.empty()) {
			icon = iconStr[0];
		}

		if (j["renderable"].contains("color")
		    && j["renderable"]["color"].is_array()) {
			auto colorArray = j["renderable"]["color"];
			color = tcod::ColorRGB {
				static_cast<uint8_t>(colorArray[0].get<int>()),
				static_cast<uint8_t>(colorArray[1].get<int>()),
				static_cast<uint8_t>(colorArray[2].get<int>())
			};
		}

		return IconRenderable { color, icon };
	}

	std::unique_ptr<AiComponent> SaveManager::ParseAiComponent(
	    const nlohmann::json& j)
	{
		if (!j.contains("ai")) {
			return nullptr;
		}

		std::string aiType = j["ai"].get<std::string>();

		if (aiType == "confused") {
			int turnsLeft = j.value("confusionTurns", 5);
			auto baseAi = std::make_unique<HostileAi>();
			return std::make_unique<ConfusedMonsterAi>(
			    turnsLeft, std::move(baseAi));
		}

		// Default to hostile for unknown or "hostile"
		return std::make_unique<HostileAi>();
	}

	std::unique_ptr<Entity> SaveManager::CreatePlayerEntity(
	    const nlohmann::json& j, pos_t pos, const std::string& name,
	    const std::string& pluralName, int stackCount,
	    const std::string& templateId, int renderPriority, bool blocker,
	    AttackerComponent attacker, DestructibleComponent destructible,
	    IconRenderable renderable, Faction faction, bool pickable,
	    bool isCorpse)
	{
		auto player = std::make_unique<Player>(
		    pos, name, blocker, attacker, destructible, renderable,
		    faction, pickable, isCorpse);

		// Restore inventory
		if (j.contains("inventory") && j["inventory"].is_array()) {
			for (const auto& itemJson : j["inventory"]) {
				auto item =
				    SaveManager::Instance().DeserializeEntity(
				        itemJson);
				if (item) {
					player->AddToInventory(std::move(item));
				}
			}
		}

		player->SetPluralName(pluralName);
		player->SetStackCount(stackCount);
		player->SetTemplateId(templateId);
		player->SetRenderPriority(renderPriority);
		return player;
	}

	std::unique_ptr<Entity> SaveManager::CreateNpcEntity(
	    const nlohmann::json& j, pos_t pos, const std::string& name,
	    const std::string& pluralName, int stackCount,
	    const std::string& templateId, int renderPriority, bool blocker,
	    AttackerComponent attacker, DestructibleComponent destructible,
	    IconRenderable renderable, Faction faction, bool pickable,
	    bool isCorpse)
	{
		auto ai = SaveManager::ParseAiComponent(j);
		if (!ai) {
			ai = std::make_unique<HostileAi>();
		}

		auto npc = std::make_unique<Npc>(
		    pos, name, blocker, attacker, destructible, renderable,
		    faction, std::move(ai), pickable, isCorpse);

		npc->SetPluralName(pluralName);
		npc->SetStackCount(stackCount);
		npc->SetTemplateId(templateId);
		npc->SetRenderPriority(renderPriority);
		return npc;
	}

	std::unique_ptr<Entity> SaveManager::RestoreItemFromTemplate(
	    const nlohmann::json& j, pos_t pos, const std::string& name,
	    const std::string& pluralName, int stackCount,
	    const std::string& templateId, int renderPriority)
	{
		if (templateId.empty()
		    || !TemplateRegistry::Instance().Get(templateId)) {
			std::cerr << "[SaveManager] WARNING: "
			             "Could not restore item: "
			          << name << " (template ID: " << templateId
			          << ")" << std::endl;
			return nullptr;
		}

		auto entity =
		    TemplateRegistry::Instance().Create(templateId, pos);

		// Restore HP if modified
		if (entity->GetDestructible() && j.contains("destructible")) {
			unsigned int hp =
			    j["destructible"]["hp"].get<unsigned int>();
			unsigned int maxHp =
			    j["destructible"]["maxHp"].get<unsigned int>();

			if (hp < maxHp) {
				entity->GetDestructible()->TakeDamage(maxHp
				                                      - hp);
			}
		}

		entity->SetPluralName(pluralName);
		entity->SetStackCount(stackCount);
		entity->SetTemplateId(templateId);
		entity->SetRenderPriority(renderPriority);
		return entity;
	}

	std::unique_ptr<Entity> SaveManager::CreateItemEntity(
	    const nlohmann::json& j, pos_t pos, const std::string& name,
	    const std::string& pluralName, int stackCount,
	    const std::string& templateId, int renderPriority, bool blocker,
	    AttackerComponent attacker, DestructibleComponent destructible,
	    IconRenderable renderable, Faction faction, bool pickable,
	    bool isCorpse)
	{
		// Try to restore from template if it has item component
		if (j.contains("hasItem") && j["hasItem"].get<bool>()) {
			auto entity = SaveManager::RestoreItemFromTemplate(
			    j, pos, name, pluralName, stackCount, templateId,
			    renderPriority);
			if (entity) {
				return entity;
			}
		}

		// Fallback: create basic entity
		auto entity = std::make_unique<BaseEntity>(
		    pos, name, blocker, attacker, destructible, renderable,
		    faction, nullptr, nullptr, pickable, isCorpse);

		entity->SetPluralName(pluralName);
		entity->SetStackCount(stackCount);
		entity->SetTemplateId(templateId);
		entity->SetRenderPriority(renderPriority);
		return entity;
	}

	void SaveManager::ExtractPlayerMetadata(
	    const nlohmann::json& engineData,
	    SaveManager::SaveMetadata& metadata)
	{
		if (!engineData.contains("player")) {
			return;
		}

		const auto& playerData = engineData["player"];
		metadata.playerName = playerData.value("name", "Unknown");

		if (!playerData.contains("destructible")) {
			return;
		}

		metadata.playerHP = playerData["destructible"].value("hp", 0);
		metadata.playerMaxHP =
		    playerData["destructible"].value("maxHp", 0);
	}

	void SaveManager::ExtractLevelMetadata(
	    const nlohmann::json& engineData,
	    SaveManager::SaveMetadata& metadata)
	{
		if (!engineData.contains("level")) {
			return;
		}

		metadata.levelName = engineData["level"].value("id", "Unknown");
	}

	SaveManager& SaveManager::Instance()
	{
		static SaveManager instance;
		return instance;
	}

	bool SaveManager::SaveGame(const Engine& engine, SaveType type)
	{
		// Don't save if player is dead or game is over
		if (engine.IsGameOver()) {
			std::cout << "[SaveManager] Cannot save - game is over"
			          << std::endl;
			return false;
		}

		try {
			// Ensure save directory exists
			fs::create_directories(kSaveDirectory);

			nlohmann::json saveData;

			// Save metadata
			saveData["version"] = "1.0.0";
			saveData["saveType"] =
			    (type == SaveType::Manual) ? "manual" : "auto";
			saveData["timestamp"] = GetTimestamp();

			// Serialize game state
			saveData["engine"] = SerializeEngine(engine);

			// Write to file
			if (WriteToFile(saveData)) {
				std::cout
				    << "[SaveManager] Game saved successfully ("
				    << (type == SaveType::Manual ? "manual"
				                                 : "auto")
				    << ")" << std::endl;
				return true;
			}
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to save game: " << e.what()
			    << std::endl;
		}

		return false;
	}

	bool SaveManager::LoadGame(Engine& engine)
	{
		if (!HasSave()) {
			std::cout << "[SaveManager] No save file found"
			          << std::endl;
			return false;
		}

		try {
			nlohmann::json saveData = ReadFromFile();

			if (saveData.empty()) {
				std::cerr << "[SaveManager] Save file is empty "
				             "or corrupted"
				          << std::endl;
				return false;
			}

			// Verify version compatibility
			if (!saveData.contains("version")) {
				std::cerr
				    << "[SaveManager] Save file missing version"
				    << std::endl;
				return false;
			}

			// Verify we have engine data
			if (!saveData.contains("engine")) {
				std::cerr << "[SaveManager] Save file missing "
				             "engine data"
				          << std::endl;
				return false;
			}

			// Deserialize game state
			bool loadSuccess =
			    DeserializeEngine(saveData["engine"], engine);

			if (loadSuccess) {
				std::cout
				    << "[SaveManager] Game loaded successfully"
				    << std::endl;
				return true;
			}

			std::cerr << "[SaveManager] Failed to "
			             "deserialize game state"
			          << std::endl;
			return false;
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to load game: " << e.what()
			    << std::endl;
			return false;
		}
	}

	bool SaveManager::HasSave() const
	{
		return fs::exists(GetSavePath());
	}

	void SaveManager::DeleteSave()
	{
		try {
			std::string path = GetSavePath();
			if (fs::exists(path)) {
				fs::remove(path);
				std::cout << "[SaveManager] Save file deleted"
				          << std::endl;
			}
		} catch (const std::exception& e) {
			std::cerr << "[SaveManager] Failed to delete save: "
			          << e.what() << std::endl;
		}
	}

	std::string SaveManager::GetSavePath() const
	{
		return kSaveDirectory + kSaveFileName;
	}

	SaveManager::SaveMetadata SaveManager::GetSaveMetadata() const
	{
		SaveMetadata metadata;
		metadata.valid = false;

		if (!HasSave()) {
			return metadata;
		}

		try {
			nlohmann::json saveData = ReadFromFile();

			if (saveData.contains("engine")) {
				const auto& engineData = saveData["engine"];
				SaveManager::ExtractPlayerMetadata(engineData,
				                                   metadata);
				SaveManager::ExtractLevelMetadata(engineData,
				                                  metadata);
			}

			if (saveData.contains("timestamp")) {
				metadata.timestamp = saveData["timestamp"];
			}

			metadata.valid = true;
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to read save metadata: "
			    << e.what() << std::endl;
		}

		return metadata;
	}

	nlohmann::json SaveManager::SerializeEngine(const Engine& engine) const
	{
		nlohmann::json j;

		// Serialize player
		const Entity* player = engine.GetPlayer();
		if (player) {
			j["player"] = SerializeEntity(*player);
		}

		// Serialize current level config
		j["level"]["id"] = engine.GetCurrentLevelId();
		j["level"]["dungeonLevel"] = engine.GetDungeonLevel();

		// Serialize message log
		nlohmann::json messages = nlohmann::json::array();
		j["messageLog"] = messages;

		// Serialize entities (excluding player)
		nlohmann::json entities = nlohmann::json::array();
		for (const auto& entity : engine.GetEntities()) {
			if (entity.get() != player) {
				entities.push_back(SerializeEntity(*entity));
			}
		}
		j["entities"] = entities;

		// Serialize map state
		j["map"] = SerializeMap(engine);

		return j;
	}

	bool SaveManager::DeserializeEngine(const nlohmann::json& j,
	                                    Engine& engine)
	{
		try {
			engine.EnsureInitialized();

			// Step 1: Load level configuration
			std::string levelId =
			    j["level"]["id"].get<std::string>();
			std::string levelPath =
			    "data/levels/" + levelId + ".json";

			// Check if level file exists
			if (!std::filesystem::exists(levelPath)) {
				std::cerr
				    << "[SaveManager] Level file not found: "
				    << levelPath << ", using dungeon_1"
				    << std::endl;
				levelPath = "data/levels/dungeon_1.json";
			}

			LevelConfig levelConfig =
			    LevelConfig::LoadFromFile(levelPath);
			engine.currentLevel_ = levelConfig;

			// Step 2: Initialize templates and spawn tables
			SaveManager::LoadTemplatesAndSpawnTables(levelConfig);

			// Step 3: Initialize engine state
			SaveManager::InitializeEngineState(engine, j);

			// Step 4: Regenerate map
			engine.GenerateMap(levelConfig.generation.width,
			                   levelConfig.generation.height);
			engine.map_->Update();

			std::cout << "[SaveManager] Map regenerated: "
			          << engine.map_->GetWidth() << "x"
			          << engine.map_->GetHeight() << std::endl;

			// Step 5: Restore player and UI
			if (!j.contains("player")) {
				std::cerr << "[SaveManager] Save file missing "
				             "player data"
				          << std::endl;
				return false;
			}

			if (!SaveManager::RestorePlayerAndUI(engine,
			                                     j["player"])) {
				return false;
			}

			// Step 6: Regenerate entities and stairs
			SaveManager::RegenerateEntitiesAndStairs(engine,
			                                         levelConfig);

			// Step 7: Recompute FOV
			if (engine.player_) {
				std::cout << "[SaveManager] Computing FOV at "
				             "player position ("
				          << engine.player_->GetPos().x << ", "
				          << engine.player_->GetPos().y << ")"
				          << std::endl;
				engine.ComputeFOV();
				engine.map_->Update();
			}

			// Step 8: Restore UI state
			engine.windowState_ = Window::MainGame;
			engine.eventHandler_ =
			    std::make_unique<MainGameEventHandler>(engine);
			engine.gameOver_ = false;
			engine.turnsSinceLastAutosave_ = 0;

			// Step 9: Add welcome back message
			auto msg = LocaleManager::Instance().GetMessage(
			    "game.welcome");
			engine.messageLog_.AddMessage(
			    "Welcome back, adventurer!", msg.color, false);

			std::cout
			    << "[SaveManager] Game state restored successfully"
			    << std::endl;
			return true;
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to restore game state: "
			    << e.what() << std::endl;

			// On failure, start new game
			std::cerr << "[SaveManager] Starting new game instead"
			          << std::endl;
			engine.NewGame();
			return false;
		}
	}

	nlohmann::json SaveManager::SerializeEntity(const Entity& entity) const
	{
		nlohmann::json j;

		// Basic properties
		j["name"] = entity.GetName();
		j["pluralName"] = entity.GetPluralName();
		j["stackCount"] = entity.GetStackCount();
		j["templateId"] = entity.GetTemplateId();
		j["pos"]["x"] = entity.GetPos().x;
		j["pos"]["y"] = entity.GetPos().y;
		j["blocker"] = entity.IsBlocker();
		j["pickable"] = entity.IsPickable();
		j["isCorpse"] = entity.IsCorpse();
		j["renderPriority"] = entity.GetRenderPriority();

		// Faction
		switch (entity.GetFaction()) {
			case Faction::PLAYER:
				j["faction"] = "player";
				break;
			case Faction::MONSTER:
				j["faction"] = "monster";
				break;
			case Faction::NEUTRAL:
				j["faction"] = "neutral";
				break;
		}

		// Components
		if (const auto* attacker = entity.GetAttacker()) {
			j["attacker"]["strength"] = attacker->GetStrength();
		}

		if (const auto* destructible = entity.GetDestructible()) {
			j["destructible"]["dexterity"] =
			    destructible->GetDexterity();
			j["destructible"]["intelligence"] =
			    destructible->GetIntelligence();
			j["destructible"]["mp"] = destructible->GetMp();
			j["destructible"]["maxMp"] = destructible->GetMaxMp();
			j["destructible"]["hp"] = destructible->GetHealth();
			j["destructible"]["maxHp"] =
			    destructible->GetMaxHealth();
			j["destructible"]["xp"] = destructible->GetXp();
			j["destructible"]["xpReward"] =
			    destructible->GetXpReward();
		}

		// Renderable
		if (const auto* renderable = entity.GetRenderable()) {
			if (const auto* iconRenderable =
			        dynamic_cast<const IconRenderable*>(
			            renderable)) {
				j["renderable"]["icon"] =
				    std::string(1, iconRenderable->GetIcon());
				j["renderable"]["color"] =
				    nlohmann::json::array(
				        { iconRenderable->GetColor().r,
				          iconRenderable->GetColor().g,
				          iconRenderable->GetColor().b });
			}
		}

		// Item component
		if (entity.GetItem()) {
			j["hasItem"] = true;

			auto allIds = TemplateRegistry::Instance().GetAllIds();
			for (const auto& id : allIds) {
				const auto* tpl =
				    TemplateRegistry::Instance().Get(id);
				if (tpl && tpl->name == entity.GetName()) {
					j["templateId"] = id;
					break;
				}
			}
		}

		// AI for NPCs
		if (dynamic_cast<const Npc*>(&entity)) {
			j["ai"] = "hostile";
		}

		// Player inventory
		if (const auto* player = dynamic_cast<const Player*>(&entity)) {
			nlohmann::json inventory = nlohmann::json::array();
			for (const auto& item : player->GetInventory()) {
				inventory.push_back(SerializeEntity(*item));
			}
			j["inventory"] = inventory;
		}

		return j;
	}

	std::unique_ptr<Entity> SaveManager::DeserializeEntity(
	    const nlohmann::json& j)
	{
		try {
			// Extract basic properties
			std::string name = j.value("name", "unknown");
			std::string pluralName =
			    j.value("pluralName", name + "s");
			int stackCount = j.value("stackCount", 1);
			std::string templateId = j.value("templateId", "");
			pos_t pos { j["pos"]["x"].get<int>(),
				    j["pos"]["y"].get<int>() };
			bool blocker = j.value("blocker", false);
			bool pickable = j.value("pickable", true);
			bool isCorpse = j.value("isCorpse", false);
			int renderPriority = j.value("renderPriority", 0);

			// Parse faction
			Faction faction = Faction::NEUTRAL;
			std::string factionStr = j.value("faction", "neutral");
			if (factionStr == "player") {
				faction = Faction::PLAYER;
			} else if (factionStr == "monster") {
				faction = Faction::MONSTER;
			}

			// Parse components
			AttackerComponent attacker =
			    SaveManager::ParseAttackerComponent(j);
			DestructibleComponent destructible =
			    SaveManager::ParseDestructibleComponent(j);
			IconRenderable renderable =
			    SaveManager::ParseRenderableComponent(j);

			// Create entity based on type
			if (faction == Faction::PLAYER) {
				return SaveManager::CreatePlayerEntity(
				    j, pos, name, pluralName, stackCount,
				    templateId, renderPriority, blocker,
				    attacker, destructible, renderable, faction,
				    pickable, isCorpse);
			}

			if (j.contains("ai")) {
				return SaveManager::CreateNpcEntity(
				    j, pos, name, pluralName, stackCount,
				    templateId, renderPriority, blocker,
				    attacker, destructible, renderable, faction,
				    pickable, isCorpse);
			}

			return SaveManager::CreateItemEntity(
			    j, pos, name, pluralName, stackCount, templateId,
			    renderPriority, blocker, attacker, destructible,
			    renderable, faction, pickable, isCorpse);
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to deserialize entity: "
			    << e.what() << std::endl;
			return nullptr;
		}
	}

	nlohmann::json SaveManager::SerializeMap(const Engine& /*engine*/) const
	{
		nlohmann::json j;

		// Map is regenerated on load (traditional roguelike behavior)
		j["note"] =
		    "Map is regenerated on load (traditional roguelike "
		    "behavior)";

		return j;
	}

	bool SaveManager::DeserializeMap(const nlohmann::json& /*j*/,
	                                 Engine& /*engine*/)
	{
		// Map is regenerated, not restored
		return true;
	}

	bool SaveManager::WriteToFile(const nlohmann::json& j) const
	{
		try {
			std::string path = GetSavePath();
			std::ofstream file(path);

			if (!file.is_open()) {
				std::cerr << "[SaveManager] Failed to open "
				             "save file for writing: "
				          << path << std::endl;
				return false;
			}

			// Write with pretty formatting
			file << std::setw(4) << j << std::endl;

			return true;
		} catch (const std::exception& e) {
			std::cerr << "[SaveManager] Write error: " << e.what()
			          << std::endl;
			return false;
		}
	}

	nlohmann::json SaveManager::ReadFromFile() const
	{
		try {
			std::string path = GetSavePath();
			std::ifstream file(path);

			if (!file.is_open()) {
				return nlohmann::json();
			}

			nlohmann::json j;
			file >> j;

			return j;
		} catch (const std::exception& e) {
			std::cerr << "[SaveManager] Read error: " << e.what()
			          << std::endl;
			return nlohmann::json();
		}
	}

	std::string SaveManager::GetTimestamp() const
	{
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);

		std::stringstream ss;
		ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

		return ss.str();
	}

} // namespace tutorial
