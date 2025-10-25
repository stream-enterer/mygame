#include "Engine.hpp"

#include "Colors.hpp"
#include "DynamicSpawnSystem.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "EventHandler.hpp"
#include "HealthBar.hpp"
#include "InventoryWindow.hpp"
#include "ItemSelectionWindow.hpp"
#include "LevelConfig.hpp"
#include "Map.hpp"
#include "MapGenerator.hpp"
#include "MenuWindow.hpp"
#include "MessageHistoryWindow.hpp"
#include "MessageLogWindow.hpp"
#include "SaveManager.hpp"
#include "StringTable.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace tutorial
{
	// Public methods
	Engine::Engine(const Configuration& config)
	    : config_(config),
	      eventHandler_(nullptr),
	      map_(nullptr),
	      messageHistoryWindow_(std::make_unique<MessageHistoryWindow>(
	          config.width, config.height, pos_t { 0, 0 }, messageLog_)),
	      messageLogWindow_(nullptr),
	      player_(nullptr),
	      healthBar_(nullptr),
	      stairs_(nullptr),
	      dungeonLevel_(1),
	      turnsSinceLastAutosave_(0),
	      context_(nullptr),
	      menuWindow_(nullptr),
	      console_(nullptr),
	      window_(nullptr),
	      windowState_(StartMenu),
	      gameOver_(false),
	      running_(true),
	      mousePos_ { 0, 0 },
	      inventoryMode_(InventoryMode::Use)
	{
		console_ = TCOD_console_new(config.width, config.height);
		if (!console_) {
			SDL_Quit();
			throw std::runtime_error("Failed to create console");
		}

		// Load BDF font if path is provided
		if (!config.fontPath.empty()) {
			try {
				tileset_ = tcod::load_bdf(config.fontPath);
			} catch (const std::exception& e) {
				TCOD_console_delete(console_);
				SDL_Quit();
				throw std::runtime_error(
				    std::string("Failed to load BDF font: ")
				    + e.what());
			}
		}

		TCOD_ContextParams params = {};
		params.tcod_version = TCOD_COMPILEDVERSION;
		params.console = console_;
		params.window_title = config.title.c_str();
		// Set tileset if loaded
		if (tileset_) {
			params.tileset = tileset_.get();
		}
		params.sdl_window_flags = SDL_WINDOW_RESIZABLE;
		params.vsync = 1;
		params.argc = 0;
		params.argv = nullptr;

		if (TCOD_context_new(&params, &context_) != TCOD_E_OK) {
			TCOD_console_delete(console_);
			SDL_Quit();
			throw std::runtime_error("Failed to create context");
		}

		window_ = TCOD_context_get_sdl_window(context_);

		// Configure viewport for centered rendering with integer
		// scaling
		viewportOptions_.tcod_version = TCOD_COMPILEDVERSION;
		viewportOptions_.keep_aspect = true;
		viewportOptions_.integer_scaling = true;
		viewportOptions_.clear_color = { 0, 0, 0,
			                         255 }; // Black letterbox
		viewportOptions_.align_x = 0.5f;        // Center horizontally
		viewportOptions_.align_y = 0.5f;        // Center vertically

		this->ShowStartMenu();
	}

	Engine::~Engine()
	{
		if (context_) {
			TCOD_context_delete(context_);
		}
		if (console_) {
			TCOD_console_delete(console_);
		}
		SDL_Quit();
	}

	void Engine::AddEventFront(Event_ptr& event)
	{
		eventQueue_.push_front(std::move(event));
	}

	void Engine::ComputeFOV()
	{
		int fovRadius = ConfigManager::Instance().GetPlayerFOVRadius();
		map_->ComputeFov(player_->GetPos(), fovRadius);
		map_->UpdateScent(
		    player_
		        ->GetPos()); // Update scent field after FOV computation
		map_->Update();
	}

	std::unique_ptr<Command> Engine::GetInput()
	{
		return eventHandler_->Dispatch();
	}

	void Engine::HandleDeathEvent(Entity& entity)
	{
		// For both player and non-player: mark for deferred removal
		// This ensures corpse spawning happens consistently
		entitiesToRemove_.push_back(&entity);

		// Player-specific handling
		if (this->IsPlayer(entity)) {
			eventHandler_ =
			    std::make_unique<GameOverEventHandler>(*this);
			eventQueue_.clear();
			gameOver_ = true;
		}
	}

	void Engine::HandleEvents()
	{
		// Remember player position before events
		pos_t playerPosBefore =
		    player_ ? player_->GetPos() : pos_t { 0, 0 };

		while (!eventQueue_.empty()) {
			auto event = std::move(eventQueue_.front());
			eventQueue_.pop_front();
			event->Execute();
		}

		// Post-processing: Update FOV if player moved
		if (player_) {
			pos_t playerPosAfter = player_->GetPos();
			if (playerPosBefore != playerPosAfter) {
				ComputeFOV();
			}
		}

		eventQueue_.clear();
		ProcessDeferredRemovals();
	}
	void Engine::LogMessage(const std::string& text, tcod::ColorRGB color,
	                        bool stack)
	{
		messageLog_.AddMessage(text, color, stack);
	}

	void Engine::EnsureInitialized()
	{
		// Initialize components that are needed regardless of whether
		// we're starting a new game or loading a saved one

		auto& cfg = ConfigManager::Instance();

		// Initialize map if not already created
		if (!map_) {
			map_ = std::make_unique<Map>(
			    config_.width,
			    config_.height - cfg.GetMapHeightOffset());
		}

		// Initialize message log window if not already created
		if (!messageLogWindow_) {
			messageLogWindow_ = std::make_unique<MessageLogWindow>(
			    cfg.GetMessageLogWidth(), cfg.GetMessageLogHeight(),
			    pos_t { cfg.GetMessageLogX(),
			            cfg.GetMessageLogY() },
			    messageLog_);
		}
	}

	void Engine::NewGame()
	{
		static bool configLoaded = false;
		if (!configLoaded) {
			ConfigManager::Instance().LoadAll();
			configLoaded = true;
		}

		// Ensure basic components are initialized
		EnsureInitialized();

		currentLevel_ =
		    LevelConfig::LoadFromFile("data/levels/dungeon_1.json");

		try {
			TemplateRegistry::Instance().Clear();

			// Load special entities (player, corpse, stairs) from
			// legacy format
			TemplateRegistry::Instance().LoadFromDirectory(
			    "data/entities");

			// Load units (monsters/NPCs) - one file per unit
			TemplateRegistry::Instance().LoadSimplifiedDirectory(
			    "data/units", "unit");

			// Load items - one file per item
			TemplateRegistry::Instance().LoadSimplifiedDirectory(
			    "data/items", "item");

			std::cout
			    << "[Engine] Loaded "
			    << TemplateRegistry::Instance().GetAllIds().size()
			    << " templates (special + units + items)"
			    << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "[Engine] FATAL: Failed to load entity "
			             "templates: "
			          << e.what() << std::endl;
			throw;
		}

		try {
			DynamicSpawnSystem::Instance().Clear();
			DynamicSpawnSystem::Instance().BuildSpawnTablesForLevel(
			    currentLevel_);
		} catch (const std::exception& e) {
			std::cerr
			    << "[Engine] FATAL: Failed to build spawn tables: "
			    << e.what() << std::endl;
			throw;
		}

		entities_.Clear();
		messageLog_.Clear();
		eventQueue_.clear();

		this->GenerateMap(currentLevel_.generation.width,
		                  currentLevel_.generation.height);

		auto rooms = map_->GetRooms();

		for (auto it = rooms.begin() + 1; it != rooms.end(); ++it) {
			entities_.PlaceItems(*it, currentLevel_.itemSpawning,
			                     currentLevel_.id);
		}

		for (auto it = rooms.begin() + 1; it != rooms.end(); ++it) {
			entities_.PlaceEntities(*it,
			                        currentLevel_.monsterSpawning,
			                        currentLevel_.id);
		}

		auto playerEntity = TemplateRegistry::Instance().Create(
		    "player", rooms[0].GetCenter());
		player_ = entities_.Spawn(std::move(playerEntity)).get();

		auto& cfg = ConfigManager::Instance();
		healthBar_ = std::make_unique<HealthBar>(
		    cfg.GetHealthBarWidth(), cfg.GetHealthBarHeight(),
		    pos_t { cfg.GetHealthBarX(), cfg.GetHealthBarY() },
		    *player_);

		// Place stairs in the center of the last room (furthest from
		// player)
		if (!rooms.empty()) {
			pos_t stairsPos = rooms.back().GetCenter();
			auto stairsEntity = TemplateRegistry::Instance().Create(
			    "stairs_down", stairsPos);
			stairs_ =
			    entities_.Spawn(std::move(stairsEntity)).get();
			std::cout << "[Engine] Placed stairs at ("
			          << stairsPos.x << ", " << stairsPos.y << ")"
			          << std::endl;
		}

		int invWidth = cfg.GetInventoryWindowWidth();
		int invHeight = cfg.GetInventoryWindowHeight();
		pos_t invPos;

		if (cfg.GetInventoryCenterOnScreen()) {
			invPos = pos_t { static_cast<int>(config_.width) / 2
				             - invWidth / 2,
				         static_cast<int>(config_.height) / 2
				             - invHeight / 2 };
		} else {
			invPos = pos_t { 0, 0 };
		}

		inventoryWindow_ = std::make_unique<InventoryWindow>(
		    invWidth, invHeight, invPos, *player_);

		this->ComputeFOV();

		auto welcomeMsg =
		    StringTable::Instance().GetMessage("game.welcome");
		messageLog_.AddMessage(welcomeMsg.text, welcomeMsg.color,
		                       welcomeMsg.stack);

		windowState_ = MainGame;
		eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
		gameOver_ = false;
		turnsSinceLastAutosave_ = 0;
	}

	void Engine::ReturnToMainGame()
	{
		if (windowState_ != MainGame) {
			eventHandler_ =
			    std::make_unique<MainGameEventHandler>(*this);
			windowState_ = MainGame;
			itemSelectionList_.clear();
		}
	}

	void Engine::ShowMessageHistory()
	{
		if (windowState_ != MessageHistory) {
			eventHandler_ =
			    std::make_unique<MessageHistoryEventHandler>(*this);
			windowState_ = MessageHistory;
		}
	}

	void Engine::ShowInventory()
	{
		if (windowState_ != Inventory) {
			eventHandler_ =
			    std::make_unique<InventoryEventHandler>(*this);
			windowState_ = Inventory;

			if (auto* invHandler =
			        dynamic_cast<InventoryEventHandler*>(
			            eventHandler_.get())) {
				invHandler->SetMode(inventoryMode_);

				if (inventoryMode_ == InventoryMode::Drop) {
					inventoryWindow_->SetTitle(
					    "Drop which item?");
				} else {
					inventoryWindow_->SetTitle("Inventory");
				}
			}

			inventoryMode_ = InventoryMode::Use;
		}
	}

	void Engine::ShowItemSelection(const std::vector<Entity*>& items)
	{
		if (windowState_ != ItemSelection) {
			itemSelectionList_ = items;

			auto& cfg = ConfigManager::Instance();
			int width = cfg.GetInventoryWindowWidth();
			int height = cfg.GetInventoryWindowHeight();
			pos_t pos;

			if (cfg.GetInventoryCenterOnScreen()) {
				pos =
				    pos_t { static_cast<int>(config_.width / 2)
					        - width / 2,
					    static_cast<int>(config_.height / 2)
					        - height / 2 };
			} else {
				pos = pos_t { 0, 0 };
			}

			itemSelectionWindow_ =
			    std::make_unique<ItemSelectionWindow>(
			        width, height, pos, itemSelectionList_,
			        "Pick up which item?");

			eventHandler_ =
			    std::make_unique<ItemSelectionEventHandler>(*this);
			windowState_ = ItemSelection;
		}
	}

	void Engine::ShowPauseMenu()
	{
		if (windowState_ != PauseMenu) {
			// Create menu window centered on screen
			int width = 40;
			int height = 20;
			pos_t pos { static_cast<int>(config_.width) / 2
				        - width / 2,
				    static_cast<int>(config_.height) / 2
				        - height / 2 };

			menuWindow_ = std::make_unique<MenuWindow>(
			    width, height, pos, "Game Menu");

			// Build menu based on game state
			menuWindow_->Clear();
			menuWindow_->AddItem(MenuAction::Continue,
			                     "Resume Game");
			menuWindow_->AddItem(MenuAction::NewGame, "New Game");
			menuWindow_->AddItem(MenuAction::SaveAndQuit,
			                     "Save and Quit");

			eventHandler_ =
			    std::make_unique<PauseMenuEventHandler>(*this);
			windowState_ = PauseMenu;
		}
	}

	void Engine::ShowLevelUpMenu()
	{
		if (windowState_ != LevelUpMenu) {
			// Create menu window centered on screen
			int width = 50;
			int height = 18;
			pos_t pos { static_cast<int>(config_.width) / 2
				        - width / 2,
				    static_cast<int>(config_.height) / 2
				        - height / 2 };

			menuWindow_ = std::make_unique<MenuWindow>(
			    width, height, pos, "Level Up!");

			// Build level-up options
			menuWindow_->Clear();
			menuWindow_->AddItem(MenuAction::LevelUpConstitution,
			                     "Constitution (+20 HP)");
			menuWindow_->AddItem(MenuAction::LevelUpStrength,
			                     "Strength (+1 attack)");
			menuWindow_->AddItem(MenuAction::LevelUpAgility,
			                     "Agility (+1 defense)");

			eventHandler_ =
			    std::make_unique<LevelUpMenuEventHandler>(*this);
			windowState_ = LevelUpMenu;
		}
	}

	void Engine::ShowStartMenu()
	{
		// Create menu window centered on screen
		int width = 40;
		int height = 20;
		pos_t pos { static_cast<int>(config_.width) / 2 - width / 2,
			    static_cast<int>(config_.height) / 2 - height / 2 };

		menuWindow_ = std::make_unique<MenuWindow>(width, height, pos,
		                                           "Main Menu");

		// Build start menu options
		menuWindow_->Clear();
		menuWindow_->AddItem(MenuAction::NewGame, "New Game");

		// Only show "Continue" if a save file exists
		if (SaveManager::Instance().HasSave()) {
			menuWindow_->AddItem(MenuAction::Continue, "Continue");
		}

		menuWindow_->AddItem(MenuAction::Quit, "Exit");

		eventHandler_ = std::make_unique<StartMenuEventHandler>(*this);
		windowState_ = StartMenu;
	}

	void Engine::MenuNavigateUp()
	{
		if (menuWindow_) {
			menuWindow_->SelectPrevious();
		}
	}

	void Engine::MenuNavigateDown()
	{
		if (menuWindow_) {
			menuWindow_->SelectNext();
		}
	}

	void Engine::MenuConfirm()
	{
		if (!menuWindow_) {
			return;
		}

		MenuAction action = menuWindow_->GetSelectedAction();

		// Handle differently based on which menu we're in
		if (windowState_ == StartMenu) {
			// Start Menu actions
			switch (action) {
				case MenuAction::NewGame:
					NewGame();
					ReturnToMainGame();
					break;

				case MenuAction::Continue:
					if (SaveManager::Instance().LoadGame(
					        *this)) {
						ReturnToMainGame();
					} else {
						// Load failed, start new game
						NewGame();
						ReturnToMainGame();
					}
					break;

				case MenuAction::Quit:
					Quit();
					break;

				case MenuAction::None:
				default:
					break;
			}
		} else if (windowState_ == PauseMenu) {
			// Pause Menu actions
			switch (action) {
				case MenuAction::Continue:
					ReturnToMainGame();
					break;

				case MenuAction::NewGame:
					NewGame();
					ReturnToMainGame();
					break;

				case MenuAction::SaveAndQuit:
					SaveManager::Instance().SaveGame(
					    *this, SaveType::Manual);
					Quit();
					break;

				case MenuAction::None:
				default:
					break;
			}
		} else if (windowState_ == LevelUpMenu) {
			// Level-up stat selection
			if (!player_ || !player_->GetDestructible()) {
				ReturnToMainGame();
				return;
			}

			auto* destructible = player_->GetDestructible();
			auto* attacker = player_->GetAttacker();

			switch (action) {
				case MenuAction::LevelUpConstitution:
					// +20 max HP and current HP
					destructible->IncreaseMaxHealth(20);
					LogMessage(
					    "Your health increases by 20 HP!",
					    { 0, 255, 0 }, false);
					break;

				case MenuAction::LevelUpStrength:
					// +1 attack power
					if (attacker) {
						attacker->IncreasePower(1);
						LogMessage(
						    "Your strength increases "
						    "by 1!",
						    { 255, 100, 0 }, false);
					}
					break;

				case MenuAction::LevelUpAgility:
					// +1 defense
					destructible->IncreaseDefense(1);
					LogMessage(
					    "Your agility increases by 1!",
					    { 100, 100, 255 }, false);
					break;

				case MenuAction::None:
				default:
					break;
			}

			// Return to game after selecting upgrade
			ReturnToMainGame();
		}
	}

	void Engine::Quit()
	{
		running_ = false;
	}

	void Engine::SetMousePos(pos_t pos)
	{
		mousePos_ = pos;
	}

	std::unique_ptr<Entity> Engine::RemoveEntity(Entity* entity)
	{
		return entities_.Remove(entity);
	}

	Entity* Engine::SpawnEntity(std::unique_ptr<Entity> entity, pos_t pos)
	{
		entity->SetPos(pos);
		return entities_.Spawn(std::move(entity), pos).get();
	}

	Entity* Engine::GetBlockingEntity(pos_t pos) const
	{
		return entities_.GetBlockingEntity(pos);
	}

	Entity* Engine::GetPlayer() const
	{
		return player_;
	}

	pos_t Engine::GetMousePos() const
	{
		return mousePos_;
	}

	TCOD_Context* Engine::GetContext() const
	{
		return context_;
	}

	const Configuration& Engine::GetConfig() const
	{
		return config_;
	}

	const TCOD_ViewportOptions& Engine::GetViewportOptions() const
	{
		return viewportOptions_;
	}

	const EntityManager& Engine::GetEntities() const
	{
		return entities_;
	}

	std::string Engine::GetCurrentLevelId() const
	{
		return currentLevel_.id;
	}

	bool Engine::IsBlocker(pos_t pos) const
	{
		if (this->GetBlockingEntity(pos)) {
			return true;
		}

		if (this->IsWall(pos)) {
			return true;
		}

		return false;
	}

	bool Engine::IsInBounds(pos_t pos) const
	{
		return map_->IsInBounds(pos);
	}

	bool Engine::IsInFov(pos_t pos) const
	{
		return map_->IsInFov(pos);
	}

	bool Engine::IsPlayer(const Entity& entity) const
	{
		return player_ == &entity;
	}

	bool Engine::IsRunning() const
	{
		return running_ && window_ != nullptr;
	}

	bool Engine::IsValid(Entity& entity) const
	{
		auto it = std::find_if(entities_.begin(), entities_.end(),
		                       [&entity](const auto& it) {
			                       return (it.get() == &entity);
		                       });

		return (it != entities_.end());
	}

	bool Engine::IsWall(pos_t pos) const
	{
		return map_->IsWall(pos);
	}

	Entity* Engine::GetClosestMonster(int x, int y, float range) const
	{
		Entity* closest = nullptr;
		float bestDistance = 1E6f;

		for (const auto& entity : entities_) {
			if (entity->GetFaction() == Faction::MONSTER
			    && entity->GetDestructible()
			    && !entity->GetDestructible()->IsDead())

			{
				float distance = entity->GetDistance(x, y);
				if (distance < bestDistance
				    && (distance <= range || range == 0.0f)) {
					bestDistance = distance;
					closest = entity.get();
				}
			}
		}

		return closest;
	}

	Entity* Engine::GetStairs() const
	{
		return stairs_;
	}

	int Engine::GetDungeonLevel() const
	{
		return dungeonLevel_;
	}

	void tutorial::Engine::NextLevel()
	{
		// Increment dungeon level
		dungeonLevel_++;

		std::cout << "[Engine] Descending to dungeon level "
		          << dungeonLevel_ << std::endl;

		LogMessage(
		    "After a rare moment of peace, you descend deeper into "
		    "the heart of the dungeon...",
		    { 255, 60, 60 }, false);

		// Determine which level config to load based on depth
		std::string levelConfigPath;
		if (dungeonLevel_ == 1) {
			levelConfigPath = "data/levels/dungeon_1.json";
		} else if (dungeonLevel_ == 2) {
			levelConfigPath = "data/levels/dungeon_2.json";
		} else {
			// For deeper levels, cycle between dungeon_1 and
			// dungeon_2 Or use dungeon_2 for all deeper levels
			// (harder difficulty)
			levelConfigPath = "data/levels/dungeon_2.json";
		}

		// Load the new level configuration
		try {
			currentLevel_ =
			    LevelConfig::LoadFromFile(levelConfigPath);
			std::cout << "[Engine] Loaded level config: "
			          << currentLevel_.id << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "[Engine] Failed to load level config: "
			          << e.what() << ", using dungeon_1"
			          << std::endl;
			currentLevel_ = LevelConfig::LoadFromFile(
			    "data/levels/dungeon_1.json");
		}

		// Rebuild spawn tables for the new level
		try {
			DynamicSpawnSystem::Instance().Clear();
			DynamicSpawnSystem::Instance().BuildSpawnTablesForLevel(
			    currentLevel_);
		} catch (const std::exception& e) {
			std::cerr
			    << "[Engine] FATAL: Failed to build spawn tables: "
			    << e.what() << std::endl;
			throw;
		}

		// Save player's inventory before clearing entities
		std::vector<std::unique_ptr<Entity>> savedInventory;
		if (player_) {
			// Cast to Player to access inventory methods
			if (auto* playerPtr = dynamic_cast<Player*>(player_)) {
				// Extract items from player's inventory
				size_t invSize = playerPtr->GetInventorySize();
				for (size_t i = 0; i < invSize; ++i) {
					auto item =
					    playerPtr->ExtractFromInventory(
					        0); // Always extract first
					if (item) {
						savedInventory.push_back(
						    std::move(item));
					}
				}
			}
		}

		// Save player stats before clearing (position is reset to first
		// room)
		std::string playerName =
		    player_ ? player_->GetName() : "player";
		DestructibleComponent savedDestructible =
		    player_ && player_->GetDestructible()
		        ? *player_->GetDestructible()
		        : DestructibleComponent { 1, 30, 30 };
		AttackerComponent savedAttacker =
		    player_ && player_->GetAttacker() ? *player_->GetAttacker()
		                                      : AttackerComponent { 5 };

		// Clear all entities (including player and stairs)
		entities_.Clear();
		eventQueue_.clear();
		entitiesToRemove_.clear();
		player_ = nullptr;
		stairs_ = nullptr;

		// Regenerate the map
		GenerateMap(currentLevel_.generation.width,
		            currentLevel_.generation.height);
		map_->Update();

		auto rooms = map_->GetRooms();
		if (rooms.empty()) {
			std::cerr << "[Engine] FATAL: No rooms generated!"
			          << std::endl;
			return;
		}

		// Spawn monsters and items in all rooms except the first
		for (auto it = rooms.begin() + 1; it != rooms.end(); ++it) {
			entities_.PlaceItems(*it, currentLevel_.itemSpawning,
			                     currentLevel_.id);
			entities_.PlaceEntities(*it,
			                        currentLevel_.monsterSpawning,
			                        currentLevel_.id);
		}

		// Recreate player in first room with saved stats
		pos_t playerPos = rooms[0].GetCenter();
		auto playerEntity = std::make_unique<Player>(
		    playerPos, playerName, true, savedAttacker,
		    savedDestructible,
		    IconRenderable { { 255, 255, 255 }, '@' }, Faction::PLAYER);

		// Restore inventory to new player entity
		for (auto& item : savedInventory) {
			playerEntity->AddToInventory(std::move(item));
		}

		player_ = entities_.Spawn(std::move(playerEntity)).get();

		// Recreate UI components that reference the player
		auto& cfg = ConfigManager::Instance();
		healthBar_ = std::make_unique<HealthBar>(
		    cfg.GetHealthBarWidth(), cfg.GetHealthBarHeight(),
		    pos_t { cfg.GetHealthBarX(), cfg.GetHealthBarY() },
		    *player_);

		int invWidth = cfg.GetInventoryWindowWidth();
		int invHeight = cfg.GetInventoryWindowHeight();
		pos_t invPos;

		if (cfg.GetInventoryCenterOnScreen()) {
			invPos = pos_t { static_cast<int>(config_.width) / 2
				             - invWidth / 2,
				         static_cast<int>(config_.height) / 2
				             - invHeight / 2 };
		} else {
			invPos = pos_t { 0, 0 };
		}

		inventoryWindow_ = std::make_unique<InventoryWindow>(
		    invWidth, invHeight, invPos, *player_);

		// Place stairs in the last room
		if (!rooms.empty()) {
			pos_t stairsPos = rooms.back().GetCenter();
			auto stairsEntity = TemplateRegistry::Instance().Create(
			    "stairs_down", stairsPos);
			stairs_ =
			    entities_.Spawn(std::move(stairsEntity)).get();
			std::cout << "[Engine] Placed stairs at ("
			          << stairsPos.x << ", " << stairsPos.y << ")"
			          << std::endl;
		}

		// Recompute FOV for new position
		ComputeFOV();
		map_->Update();

		// Log the new level
		LogMessage("Welcome to dungeon level "
		               + std::to_string(dungeonLevel_) + "!",
		           { 255, 255, 0 }, false);

		// Return to game state (in case we were in a menu)
		windowState_ = MainGame;
		eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
	}

	bool Engine::PickATile(int* x, int* y, float maxRange,
	                       std::function<bool(int, int)> validator,
	                       TargetingType targetingType, float radius)
	{
		// Remember previous window state to restore later
		Window previousWindowState = windowState_;

		// Enter targeting mode (blocks inventory/other UI)
		windowState_ = MainGame;

		// Render current game state before targeting
		this->Render();

		// Create targeting cursor (handles all targeting logic)
		TargetingCursor cursor(*this, maxRange, targetingType, radius);

		// Let cursor handle all input and selection with validator
		bool result = cursor.SelectTile(x, y, validator);

		// Restore window state
		windowState_ = previousWindowState;

		// Cursor destructor automatically restores console state
		return result;
	}

	Entity* Engine::GetActor(int x, int y) const
	{
		for (const auto& entity : entities_) {
			if (entity->GetPos().x == x && entity->GetPos().y == y
			    && entity->GetDestructible()
			    && !entity->GetDestructible()->IsDead()) {
				return entity.get();
			}
		}
		return nullptr;
	}

	int Engine::GetMaxRenderPriorityAtPosition(pos_t pos) const
	{
		int maxPriority = 0;
		for (const auto& entity : entities_) {
			if (entity->GetPos() == pos) {
				maxPriority = std::max(
				    maxPriority, entity->GetRenderPriority());
			}
		}
		return maxPriority;
	}

	void Engine::DealDamage(Entity& target, unsigned int damage)
	{
		target.GetDestructible()->TakeDamage(damage);

		if (target.GetDestructible()->IsDead()) {
			auto action = DieAction(*this, target);
			std::unique_ptr<Event> event =
			    std::make_unique<DieAction>(action);
			AddEventFront(event);
		}
	}

	void Engine::Render()
	{
		TCOD_console_clear(console_);

		if (windowState_ == StartMenu) {
			// Start menu - only render the menu on black background
			if (menuWindow_) {
				menuWindow_->Render(console_);
			}
		} else if (windowState_ == MainGame) {
			map_->Render(console_);

			// Render all entities in FOV
			for (const auto& entity : entities_) {
				const auto pos = entity->GetPos();
				if (map_->IsInFov(pos)) {
					const auto& renderable =
					    entity->GetRenderable();
					renderable->Render(console_, pos);
				}
			}

			// Only render health bar if player exists
			if (player_ && healthBar_) {
				healthBar_->Render(console_);
			}

			messageLogWindow_->Render(console_);
			messageLogWindow_->RenderMouseLook(console_, *this);
		} else if (windowState_ == MessageHistory) {
			messageHistoryWindow_->Render(console_);
		} else if (windowState_ == Inventory) {
			map_->Render(console_);

			for (const auto& entity : entities_) {
				const auto pos = entity->GetPos();
				if (map_->IsInFov(pos)) {
					const auto& renderable =
					    entity->GetRenderable();
					renderable->Render(console_, pos);
				}
			}

			// Only render health bar if player exists
			if (player_ && healthBar_) {
				healthBar_->Render(console_);
			}

			messageLogWindow_->Render(console_);

			inventoryWindow_->Render(console_);
		} else if (windowState_ == ItemSelection) {
			map_->Render(console_);

			for (const auto& entity : entities_) {
				const auto pos = entity->GetPos();
				if (map_->IsInFov(pos)) {
					const auto& renderable =
					    entity->GetRenderable();
					renderable->Render(console_, pos);
				}
			}

			// Only render health bar if player exists
			if (player_ && healthBar_) {
				healthBar_->Render(console_);
			}

			messageLogWindow_->Render(console_);

			itemSelectionWindow_->Render(console_);
		} else if (windowState_ == PauseMenu) {
			// Render game in background
			map_->Render(console_);

			for (const auto& entity : entities_) {
				const auto pos = entity->GetPos();
				if (map_->IsInFov(pos)) {
					const auto& renderable =
					    entity->GetRenderable();
					renderable->Render(console_, pos);
				}
			}

			if (player_ && healthBar_) {
				healthBar_->Render(console_);
			}

			messageLogWindow_->Render(console_);

			// Render menu on top
			if (menuWindow_) {
				menuWindow_->Render(console_);
			}
		} else if (windowState_ == LevelUpMenu) {
			// Render game in background
			map_->Render(console_);

			for (const auto& entity : entities_) {
				const auto pos = entity->GetPos();
				if (map_->IsInFov(pos)) {
					const auto& renderable =
					    entity->GetRenderable();
					renderable->Render(console_, pos);
				}
			}

			if (player_ && healthBar_) {
				healthBar_->Render(console_);
			}

			messageLogWindow_->Render(console_);

			// Render level-up menu on top
			if (menuWindow_) {
				menuWindow_->Render(console_);
			}
		}

		TCOD_context_present(context_, console_, &viewportOptions_);
	}

	void Engine::RenderGameUI(TCOD_Console* targetConsole) const
	{
		// Render health bar if player exists
		if (player_ && healthBar_) {
			healthBar_->Render(targetConsole);
		}

		// Render message log
		if (messageLogWindow_) {
			messageLogWindow_->Render(targetConsole);
			messageLogWindow_->RenderMouseLook(targetConsole,
			                                   *this);
		}
	}

	// Private methods
	void Engine::AddEvent(Event_ptr& event)
	{
		eventQueue_.push_back(std::move(event));
	}

	void Engine::GenerateMap(int width, int height)
	{
		Map::Generator generator({ currentLevel_.generation.maxRooms,
		                           currentLevel_.generation.minRoomSize,
		                           currentLevel_.generation.maxRoomSize,
		                           width, height });

		map_->Generate(generator);
		map_->Update();
	}

	void Engine::ProcessDeferredRemovals()
	{
		for (Entity* entity : entitiesToRemove_) {
			std::string corpseName =
			    "remains of " + entity->GetName();
			pos_t corpsePos = entity->GetPos();

			// Create corpse using factory template
			auto corpse = TemplateRegistry::Instance().Create(
			    "corpse", corpsePos);

			if (corpse) {
				// Override the generic name with specific
				// corpse name (can't change through template
				// since name is per-instance) Note: This
				// requires adding a SetName() method to Entity
				// For now, we'll keep the factory-created
				// corpse as-is
				// TODO: Add Entity::SetName() if we want custom
				// corpse names

				// Set corpse priority to -1 so it renders below
				// any items at this position (Items have
				// default priority 0 or higher within the ITEMS
				// layer)
				corpse->SetRenderPriority(-1);
				SpawnEntity(std::move(corpse), corpsePos);
			}

			// CRITICAL FIX: Nullify player pointer if we're
			// removing the player
			if (entity == player_) {
				player_ = nullptr;
			}

			// Now safe to remove the entity
			RemoveEntity(entity);
		}

		entitiesToRemove_.clear();
	}
} // namespace tutorial
