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
#include "Map.hpp"
#include "StringTable.hpp"
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
				// Don't delete - might be a read error, not a
				// corrupt file
				return false;
			}

			// Verify version compatibility
			if (!saveData.contains("version")) {
				std::cerr
				    << "[SaveManager] Save file missing version"
				    << std::endl;
				// Don't delete - let user handle old saves
				// manually
				return false;
			}

			// Verify we have engine data
			if (!saveData.contains("engine")) {
				std::cerr << "[SaveManager] Save file missing "
				             "engine data"
				          << std::endl;
				// Don't delete - corrupted but might be
				// recoverable
				return false;
			}

			// Deserialize game state
			bool loadSuccess =
			    DeserializeEngine(saveData["engine"], engine);

			if (loadSuccess) {
				std::cout
				    << "[SaveManager] Game loaded successfully"
				    << std::endl;

				// === OPTION: Don't delete, just mark as loaded
				// === Save will be overwritten on next
				// save/autosave This means save persists until
				// overwritten Safer against crashes, but allows
				// save-scumming by copying the file
				// DeleteSave();  // Comment out
				// === END OPTION ===

				return true;
			} else {
				std::cerr << "[SaveManager] Failed to "
				             "deserialize game state"
				          << std::endl;
				// Don't delete save - deserialization failed,
				// preserve save for debugging
				return false;
			}
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to load game: " << e.what()
			    << std::endl;

			// === DON'T DELETE ON EXCEPTION ===
			// Save file might be fine, just a temporary error
			// Let user try again or manually delete
			// === END ===

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

				if (engineData.contains("player")) {
					const auto& playerData =
					    engineData["player"];
					metadata.playerName =
					    playerData.value("name", "Unknown");

					if (playerData.contains(
					        "destructible")) {
						metadata.playerHP =
						    playerData["destructible"]
						        .value("hp", 0);
						metadata.playerMaxHP =
						    playerData["destructible"]
						        .value("maxHp", 0);
					}
				}

				if (engineData.contains("level")) {
					metadata.levelName =
					    engineData["level"].value(
					        "id", "Unknown");
				}
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
		// Note: We store the level ID and regenerate on load
		// This maintains roguelike tradition of not saving exact map
		// state Access currentLevel_ through Engine (it's a private
		// member, but we need a getter) For now, we'll add a public
		// getter to Engine
		j["level"]["id"] = engine.GetCurrentLevelId();
		j["level"]["dungeonLevel"] = engine.GetDungeonLevel();

		// Serialize message log
		nlohmann::json messages = nlohmann::json::array();
		for (const auto& msg : engine.GetMessageLog().GetMessages()) {
			nlohmann::json msgJson;
			msgJson["text"] = msg.text;
			msgJson["color"] = nlohmann::json::array(
			    { msg.color.r, msg.color.g, msg.color.b });
			msgJson["count"] = msg.count;
			messages.push_back(msgJson);
		}
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
			// Step 1: Load the level configuration
			std::string levelId =
			    j["level"]["id"].get<std::string>();
			std::string levelPath =
			    "data/levels/" + levelId + ".json";

			// Restore dungeon level
			if (j["level"].contains("dungeonLevel")) {
				engine.dungeonLevel_ =
				    j["level"]["dungeonLevel"].get<int>();
				std::cout
				    << "[SaveManager] Restored dungeon level: "
				    << engine.dungeonLevel_ << std::endl;
			} else {
				engine.dungeonLevel_ =
				    1; // Default to level 1 for old saves
			}

			// Check if level file exists, fallback to dungeon_1 if
			// not
			if (!std::filesystem::exists(levelPath)) {
				std::cerr
				    << "[SaveManager] Level file not found: "
				    << levelPath << ", using dungeon_1"
				    << std::endl;
				levelPath = "data/levels/dungeon_1.json";
			}

			LevelConfig levelConfig =
			    LevelConfig::LoadFromFile(levelPath);
			engine.currentLevel_ = levelConfig; // Store in engine

			// Initialize entity templates and spawn tables
			try {
				TemplateRegistry::Instance().Clear();

				// Load special entities (player, corpse,
				// stairs) from legacy format
				TemplateRegistry::Instance().LoadFromDirectory(
				    "data/entities");

				// Load units (monsters/NPCs) - one file per
				// unit
				TemplateRegistry::Instance()
				    .LoadSimplifiedDirectory("data/units",
				                             "unit");

				// Load items - one file per item
				TemplateRegistry::Instance()
				    .LoadSimplifiedDirectory("data/items",
				                             "item");

				std::cout
				    << "[SaveManager] Loaded "
				    << TemplateRegistry::Instance()
				           .GetAllIds()
				           .size()
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
				DynamicSpawnSystem::Instance()
				    .BuildSpawnTablesForLevel(levelConfig);
			} catch (const std::exception& e) {
				std::cerr << "[SaveManager] FATAL: Failed to "
				             "build spawn tables: "
				          << e.what() << std::endl;
				throw;
			}

			// Step 2: Initialize the engine with fresh state
			// Clear existing entities and message log
			engine.entities_.Clear();
			engine.messageLog_.Clear();
			engine.eventQueue_.clear();
			engine.entitiesToRemove_.clear();

			// Step 3: Regenerate the map structure (for dimensions
			// and FOV map)
			engine.GenerateMap(levelConfig.generation.width,
			                   levelConfig.generation.height);

			// Step 3b: Restore map state from save (tiles and
			// explored state)
			if (j.contains("map")) {
				if (!DeserializeMap(j["map"], engine)) {
					std::cerr << "[SaveManager] Failed to "
					             "restore map state, using "
					             "generated map"
					          << std::endl;
					// Continue anyway - we have a valid
					// generated map structure
				}
			} else {
				std::cout
				    << "[SaveManager] No map data in save (old "
				       "save format?), using generated map"
				    << std::endl;
			}

			engine.map_->Update();

			std::cout << "[SaveManager] Map structure: "
			          << engine.map_->GetWidth() << "x"
			          << engine.map_->GetHeight() << std::endl;

			// Step 4: Restore the player
			if (j.contains("player")) {
				auto playerEntity =
				    DeserializeEntity(j["player"]);
				if (!playerEntity) {
					std::cerr << "[SaveManager] Failed to "
					             "restore player"
					          << std::endl;
					return false;
				}

				// Player position was already set by
				// DeserializeEntity No need to override it -
				// the map layout is restored exactly

				engine.player_ =
				    engine.entities_
				        .Spawn(std::move(playerEntity))
				        .get();

				std::cout
				    << "[SaveManager] Player restored at ("
				    << engine.player_->GetPos().x << ", "
				    << engine.player_->GetPos().y << ")"
				    << std::endl;

				// Step 4.5: Create UI components that depend on
				// player
				auto& cfg = ConfigManager::Instance();

				engine.healthBar_ = std::make_unique<HealthBar>(
				    cfg.GetHealthBarWidth(),
				    cfg.GetHealthBarHeight(),
				    pos_t { cfg.GetHealthBarX(),
				            cfg.GetHealthBarY() },
				    *engine.player_);

				int invWidth = cfg.GetInventoryWindowWidth();
				int invHeight = cfg.GetInventoryWindowHeight();
				pos_t invPos;

				if (cfg.GetInventoryCenterOnScreen()) {
					invPos =
					    pos_t { static_cast<int>(
						        engine.config_.width)
						            / 2
						        - invWidth / 2,
						    static_cast<int>(
						        engine.config_.height)
						            / 2
						        - invHeight / 2 };
				} else {
					invPos = pos_t { 0, 0 };
				}

				engine.inventoryWindow_ =
				    std::make_unique<InventoryWindow>(
				        invWidth, invHeight, invPos,
				        *engine.player_);
			} else {
				std::cerr << "[SaveManager] Save file missing "
				             "player data"
				          << std::endl;
				return false;
			}

			// Step 5: Restore entities from save (monsters, items,
			// stairs, corpses)
			if (j.contains("entities")) {
				std::cout << "[SaveManager] Restoring entities "
				             "from save..."
				          << std::endl;

				const auto& entities = j["entities"];
				int restoredCount = 0;

				for (const auto& entityJson : entities) {
					auto entity =
					    DeserializeEntity(entityJson);
					if (entity) {
						// Check if this is the stairs
						// entity
						if (entity->GetName()
						    == "stairs") {
							engine.stairs_ =
							    engine.entities_
							        .Spawn(
							            std::move(
							                entity))
							        .get();
						} else {
							engine.entities_.Spawn(
							    std::move(entity));
						}
						restoredCount++;
					}
				}

				std::cout << "[SaveManager] Restored "
				          << restoredCount
				          << " entities from save" << std::endl;

				// Verify stairs were restored
				if (!engine.stairs_) {
					std::cerr
					    << "[SaveManager] WARNING: No "
					       "stairs found in save data!"
					    << std::endl;
				}
			} else {
				std::cerr
				    << "[SaveManager] No entities in save data!"
				    << std::endl;
				return false;
			}

			// Step 6: Recompute FOV for the restored player
			// position
			if (engine.player_) {
				std::cout << "[SaveManager] Computing FOV at "
				             "player position ("
				          << engine.player_->GetPos().x << ", "
				          << engine.player_->GetPos().y << ")"
				          << std::endl;
				engine.ComputeFOV();
				engine.map_->Update(); // Update map after FOV
				                       // computation
			}

			// Step 7: Restore UI state
			engine.menuStack_.Clear();
			engine.inMainGame_ = true;
			engine.eventHandler_ =
			    std::make_unique<MainGameEventHandler>(engine);
			engine.gameOver_ = false;
			engine.turnsSinceLastAutosave_ = 0;

			// Step 8: Restore message log
			if (j.contains("messageLog")
			    && j["messageLog"].is_array()) {
				for (const auto& msgJson : j["messageLog"]) {
					std::string text =
					    msgJson.value("text", "");
					int count = msgJson.value("count", 1);

					tcod::ColorRGB color {
						255, 255, 255
					}; // Default white
					if (msgJson.contains("color")
					    && msgJson["color"].is_array()) {
						auto colorArray =
						    msgJson["color"];
						color = tcod::ColorRGB {
							static_cast<uint8_t>(
							    colorArray[0]
							        .get<int>()),
							static_cast<uint8_t>(
							    colorArray[1]
							        .get<int>()),
							static_cast<uint8_t>(
							    colorArray[2]
							        .get<int>())
						};
					}

					// Restore message with its count
					for (int i = 0; i < count; ++i) {
						engine.messageLog_.AddMessage(
						    text, color,
						    true); // stack=true
					}
				}
				std::cout << "[SaveManager] Restored "
				          << j["messageLog"].size()
				          << " messages" << std::endl;
			} else {
				std::cout << "[SaveManager] No message log in "
				             "save (old save format?)"
				          << std::endl;
			}

			std::cout
			    << "[SaveManager] Game state restored successfully"
			    << std::endl;
			return true;

		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to restore game state: "
			    << e.what() << std::endl;

			// On failure, start a new game so the player isn't
			// stuck
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
			j["destructible"]["mana"] = destructible->GetMana();
			j["destructible"]["maxMana"] =
			    destructible->GetMaxMana();
			j["destructible"]["hp"] = destructible->GetHealth();
			j["destructible"]["maxHp"] =
			    destructible->GetMaxHealth();
			j["destructible"]["xp"] = destructible->GetXp();
			j["destructible"]["xpReward"] =
			    destructible->GetXpReward();
		}

		// Renderable (store icon and color)
		if (const auto* renderable = entity.GetRenderable()) {
			// Cast to IconRenderable to access icon and color
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

		// Item component (if entity is an item)
		if (entity.GetItem()) {
			j["hasItem"] = true;

			// Store template ID so we can recreate with proper Item
			// component
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

		// Special handling for NPCs with AI
		if (dynamic_cast<const Npc*>(&entity)) {
			// For now, we'll save all NPCs as "hostile"
			// TODO: Detect confused AI and save original AI type +
			// turns
			j["ai"] = "hostile";
		}

		// Special handling for Player inventory
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
			AttackerComponent attacker { 1 }; // Default STR = 1
			if (j.contains("attacker")) {
				// Try new format first (strength), fall back to
				// old format (power)
				unsigned int strength = 1;
				if (j["attacker"].contains("strength")) {
					strength = j["attacker"]["strength"]
					               .get<unsigned int>();
				} else if (j["attacker"].contains("power")) {
					// Backwards compatibility with old
					// saves
					strength = j["attacker"]["power"]
					               .get<unsigned int>();
				}
				attacker = AttackerComponent { strength };
			}

			DestructibleComponent destructible {
				1, 1, 1
			}; // Default DEX = 1
			if (j.contains("destructible")) {
				// Try new format first (dexterity), fall back
				// to old format (defense)
				unsigned int dexterity = 1;
				if (j["destructible"].contains("dexterity")) {
					dexterity =
					    j["destructible"]["dexterity"]
					        .get<unsigned int>();
				} else if (j["destructible"].contains(
				               "defense")) {
					// Backwards compatibility with old
					// saves
					dexterity = j["destructible"]["defense"]
					                .get<unsigned int>();
				}

				unsigned int maxHp = j["destructible"]["maxHp"]
				                         .get<unsigned int>();
				unsigned int hp =
				    j["destructible"]["hp"].get<unsigned int>();
				destructible =
				    DestructibleComponent { dexterity, maxHp,
					                    hp };

				// Restore XP data if present
				if (j["destructible"].contains("xp")) {
					unsigned int xp =
					    j["destructible"]["xp"]
					        .get<unsigned int>();
					destructible.AddXp(xp);
				}
				if (j["destructible"].contains("xpReward")) {
					unsigned int xpReward =
					    j["destructible"]["xpReward"]
					        .get<unsigned int>();
					destructible.SetXpReward(xpReward);
				}

				// Restore mana/INT data if present (new format)
				if (j["destructible"].contains(
				        "intelligence")) {
					unsigned int intelligence =
					    j["destructible"]["intelligence"]
					        .get<unsigned int>();
					// Set INT by increasing from base (1)
					if (intelligence > 1) {
						destructible
						    .IncreaseIntelligence(
						        intelligence - 1);
					}
				}
				if (j["destructible"].contains("mana")) {
					unsigned int mana =
					    j["destructible"]["mana"]
					        .get<unsigned int>();
					unsigned int currentMana =
					    destructible.GetMana();
					if (mana < currentMana) {
						destructible.SpendMana(
						    currentMana - mana);
					} else if (mana > currentMana) {
						destructible.RestoreMana(
						    mana - currentMana);
					}
				}
			}

			// Parse renderable
			char icon = '@';
			tcod::ColorRGB color { 255, 255, 255 };
			if (j.contains("renderable")) {
				std::string iconStr =
				    j["renderable"]["icon"].get<std::string>();
				if (!iconStr.empty()) {
					icon = iconStr[0];
				}

				if (j["renderable"].contains("color")
				    && j["renderable"]["color"].is_array()) {
					auto colorArray =
					    j["renderable"]["color"];
					color = tcod::ColorRGB {
						static_cast<uint8_t>(
						    colorArray[0].get<int>()),
						static_cast<uint8_t>(
						    colorArray[1].get<int>()),
						static_cast<uint8_t>(
						    colorArray[2].get<int>())
					};
				}
			}

			IconRenderable renderable { color, icon };

			// Create the appropriate entity type based on faction
			// and AI
			if (faction == Faction::PLAYER) {
				// Create Player entity
				auto player = std::make_unique<Player>(
				    pos, name, blocker, attacker, destructible,
				    renderable, faction, pickable, isCorpse);

				// Restore inventory if present
				if (j.contains("inventory")
				    && j["inventory"].is_array()) {
					for (const auto& itemJson :
					     j["inventory"]) {
						auto item =
						    DeserializeEntity(itemJson);
						if (item) {
							player->AddToInventory(
							    std::move(item));
						}
					}
				}

				player->SetRenderPriority(renderPriority);
				return player;
			} else if (j.contains("ai")) {
				// Create NPC with AI
				std::string aiType = j["ai"].get<std::string>();
				std::unique_ptr<AiComponent> ai = nullptr;

				if (aiType == "hostile") {
					ai = std::make_unique<HostileAi>();
				} else if (aiType == "confused") {
					// For confused AI, we need to recreate
					// the wrapped AI For now, default to
					// hostile as the base
					// TODO: Save/restore the original AI
					// type and confusion turns
					int turnsLeft =
					    j.value("confusionTurns", 5);
					auto baseAi =
					    std::make_unique<HostileAi>();
					ai =
					    std::make_unique<ConfusedMonsterAi>(
					        turnsLeft, std::move(baseAi));
				} else {
					// Default to hostile if unknown
					ai = std::make_unique<HostileAi>();
				}

				auto npc = std::make_unique<Npc>(
				    pos, name, blocker, attacker, destructible,
				    renderable, faction, std::move(ai),
				    pickable, isCorpse);

				npc->SetRenderPriority(renderPriority);
				return npc;
			} else {
				// Check if this entity has an item component
				if (j.contains("hasItem")
				    && j["hasItem"].get<bool>()) {
					// Check if we have a template ID
					std::string templateId =
					    j.value("templateId", "");

					if (!templateId.empty()
					    && TemplateRegistry::Instance().Get(
					        templateId)) {
						// Create from template to get
						// proper Item component
						auto entity =
						    TemplateRegistry::Instance()
						        .Create(templateId,
						                pos);

						// Restore HP if it was modified
						// (for damaged items)
						if (entity->GetDestructible()
						    && j.contains(
						        "destructible")) {
							unsigned int hp =
							    j["destructible"]
							     ["hp"]
							         .get<
							             unsigned int>();
							unsigned int maxHp =
							    j["destructible"]
							     ["maxHp"]
							         .get<
							             unsigned int>();

							if (hp < maxHp) {
								entity
								    ->GetDestructible()
								    ->TakeDamage(
								        maxHp
								        - hp);
							}
						}

						entity->SetRenderPriority(
						    renderPriority);
						return entity;
					} else {
						std::cerr
						    << "[SaveManager] WARNING: "
						       "Could not restore "
						       "item: "
						    << name << " (template ID: "
						    << templateId << ")"
						    << std::endl;
					}
				}

				// Create basic entity (corpses, or items that
				// couldn't be restored)
				auto entity = std::make_unique<BaseEntity>(
				    pos, name, blocker, attacker, destructible,
				    renderable, faction, nullptr, pickable,
				    isCorpse);

				entity->SetRenderPriority(renderPriority);
				return entity;
			}
		} catch (const std::exception& e) {
			std::cerr
			    << "[SaveManager] Failed to deserialize entity: "
			    << e.what() << std::endl;
			return nullptr;
		}
	}

	nlohmann::json SaveManager::SerializeMap(const Engine& engine) const
	{
		nlohmann::json j;

		const Map& map = engine.GetMap();

		// Save map dimensions
		j["width"] = map.GetWidth();
		j["height"] = map.GetHeight();

		// Save all tiles
		nlohmann::json tiles = nlohmann::json::array();
		const auto& tileData = map.GetTiles();

		for (const auto& tile : tileData) {
			nlohmann::json tileJson;

			// Save tile type
			switch (tile.type) {
				case TileType::FLOOR:
					tileJson["type"] = "floor";
					break;
				case TileType::WALL:
					tileJson["type"] = "wall";
					break;
				default:
					tileJson["type"] = "wall";
					break;
			}

			// Save exploration state
			tileJson["explored"] = tile.explored;

			// Save scent value for monster AI
			tileJson["scent"] = tile.scent;

			tiles.push_back(tileJson);
		}

		j["tiles"] = tiles;
		j["currentScentValue"] = map.GetCurrentScentValue();

		// Save rooms for reference (used by stairs placement, etc.)
		nlohmann::json rooms = nlohmann::json::array();
		for (const auto& room : map.GetRooms()) {
			nlohmann::json roomJson;
			pos_t origin = room.GetOrigin();
			pos_t end = room.GetEnd();
			roomJson["x1"] = origin.x;
			roomJson["y1"] = origin.y;
			roomJson["x2"] = end.x;
			roomJson["y2"] = end.y;
			rooms.push_back(roomJson);
		}
		j["rooms"] = rooms;

		return j;
	}

	bool SaveManager::DeserializeMap(const nlohmann::json& j,
	                                 Engine& engine)
	{
		try {
			if (!j.contains("tiles") || !j.contains("width")
			    || !j.contains("height")) {
				std::cerr << "[SaveManager] Map data missing "
				             "required fields"
				          << std::endl;
				return false;
			}

			int width = j["width"].get<int>();
			int height = j["height"].get<int>();

			Map& map = engine.GetMap();

			// Verify dimensions match
			if (map.GetWidth() != width
			    || map.GetHeight() != height) {
				std::cerr
				    << "[SaveManager] Map dimension mismatch"
				    << std::endl;
				return false;
			}

			// Restore tiles
			const auto& tiles = j["tiles"];
			if (tiles.size()
			    != static_cast<size_t>(width * height)) {
				std::cerr << "[SaveManager] Tile count mismatch"
				          << std::endl;
				return false;
			}

			for (size_t i = 0; i < tiles.size(); ++i) {
				const auto& tileJson = tiles[i];
				int x = i % width;
				int y = i / width;
				pos_t pos { x, y };

				// Restore tile type
				std::string typeStr =
				    tileJson["type"].get<std::string>();
				TileType type = (typeStr == "floor")
				                    ? TileType::FLOOR
				                    : TileType::WALL;
				map.SetTileType(pos, type);

				// Restore exploration state
				bool explored =
				    tileJson.value("explored", false);
				map.SetExplored(pos, explored);

				// Note: Scent values are restored but will be
				// overwritten as player moves We save them for
				// completeness but they decay naturally
			}

			// Restore rooms (needed for game logic)
			if (j.contains("rooms")) {
				std::vector<Room> restoredRooms;
				for (const auto& roomJson : j["rooms"]) {
					int x1 = roomJson["x1"].get<int>();
					int y1 = roomJson["y1"].get<int>();
					int x2 = roomJson["x2"].get<int>();
					int y2 = roomJson["y2"].get<int>();

					// Room constructor takes (origin,
					// width, height)
					pos_t origin { x1, y1 };
					int width = x2 - x1;
					int height = y2 - y1;

					Room room(origin, width, height);
					restoredRooms.push_back(room);
				}
				map.SetRooms(restoredRooms);
				std::cout << "[SaveManager] Restored "
				          << restoredRooms.size() << " rooms"
				          << std::endl;
			}

			std::cout
			    << "[SaveManager] Map state restored successfully"
			    << std::endl;
			return true;

		} catch (const std::exception& e) {
			std::cerr << "[SaveManager] Failed to deserialize map: "
			          << e.what() << std::endl;
			return false;
		}
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

			// Write with pretty formatting (4 spaces, easier to
			// debug/edit)
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
