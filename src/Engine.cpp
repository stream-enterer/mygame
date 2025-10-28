#include "Engine.hpp"

#include "Colors.hpp"
#include "DynamicSpawnSystem.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "EventHandler.hpp"
#include "HealthBar.hpp"
#include "LevelConfig.hpp"
#include "Map.hpp"
#include "MapGenerator.hpp"
#include "Menu.hpp"
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
	      console_(nullptr),
	      window_(nullptr),
	      gameState_(GameState::InMenu),
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

	void Engine::PushMenu(std::unique_ptr<Menu> menu)
	{
		menuStack_.push_back(std::move(menu));
		gameState_ = GameState::InMenu;
		eventHandler_ = std::make_unique<MenuEventHandler>(*this);
	}

	void Engine::PopMenu()
	{
		if (!menuStack_.empty()) {
			menuStack_.pop_back();
		}

		if (menuStack_.empty()) {
			gameState_ = GameState::MainGame;
			eventHandler_ =
			    std::make_unique<MainGameEventHandler>(*this);
		}
	}

	GameState Engine::GetGameState() const
	{
		return gameState_;
	}

	void Engine::HandleMenuInput(SDL_Keycode key, char character)
	{
		if (menuStack_.empty()) {
			return;
		}

		size_t menuCountBefore = menuStack_.size();
		bool shouldClose =
		    menuStack_.back()->HandleInput(key, character, *this);
		size_t menuCountAfter = menuStack_.size();

		// Only close if no sub-menu was pushed
		if (shouldClose && menuCountBefore == menuCountAfter) {
			PopMenu();
		}
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

		this->ComputeFOV();

		auto welcomeMsg =
		    StringTable::Instance().GetMessage("game.welcome");
		messageLog_.AddMessage(welcomeMsg.text, welcomeMsg.color,
		                       welcomeMsg.stack);

		gameState_ = GameState::MainGame;
		eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
		gameOver_ = false;
		turnsSinceLastAutosave_ = 0;
	}

	void Engine::ReturnToMainGame()
	{
		menuStack_.clear();
		gameState_ = GameState::MainGame;
		eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
		itemSelectionList_.clear();
	}

	void Engine::ShowMessageHistory()
	{
		if (gameState_ != GameState::MessageHistory) {
			gameState_ = GameState::MessageHistory;
			eventHandler_ =
			    std::make_unique<MessageHistoryEventHandler>(*this);
		}
	}

	void Engine::ShowStartMenu()
	{
		auto menu = std::make_unique<Menu>();
		menu->SetAllowEscape(false); // Can't escape start menu

		// Only show Continue if save exists
		if (SaveManager::Instance().HasSave()) {
			menu->AddItem("Continue", [](Engine& e) {
				if (SaveManager::Instance().LoadGame(e)) {
					e.ReturnToMainGame();
				} else {
					e.NewGame();
					e.ReturnToMainGame();
				}
			});
		}

		menu->AddItem("New Game", [](Engine& e) {
			// If save exists, need confirmation
			if (SaveManager::Instance().HasSave()) {
				auto confirmMenu = std::make_unique<Menu>();
				confirmMenu->AddItem(
				    "Yes - Start New Game", [](Engine& e) {
					    SaveManager::Instance()
					        .DeleteSave();
					    e.PopMenu(); // Close confirmation
					    e.ShowClassSelection();
				    });
				confirmMenu->AddItem(
				    "No - Return to Menu", [](Engine& e) {
					    e.PopMenu(); // Close confirmation
				    });
				e.PushMenu(std::move(confirmMenu));
			} else {
				e.ShowClassSelection();
			}
		});

		menu->AddItem("Quit", [](Engine& e) { e.Quit(); });

		PushMenu(std::move(menu));
	}

	void Engine::ShowClassSelection()
	{
		PopMenu(); // Close previous menu (start menu or confirmation)

		auto menu = std::make_unique<Menu>();

		menu->AddItem("Warrior - Tough melee fighter", [](Engine& e) {
			e.PopMenu();
			e.NewGame();
			e.ReturnToMainGame();
		});

		menu->AddItem("Rogue - Swift and deadly", [](Engine& e) {
			e.PopMenu();
			e.NewGame();
			e.ReturnToMainGame();
		});

		menu->AddItem("Mage - Master of magic", [](Engine& e) {
			e.PopMenu();
			e.NewGame();
			e.ReturnToMainGame();
		});

		PushMenu(std::move(menu));
	}

	void Engine::ShowPauseMenu()
	{
		auto menu = std::make_unique<Menu>();

		menu->AddItem("Resume Game", [](Engine& e) { e.PopMenu(); });

		menu->AddItem("Save and Quit", [](Engine& e) {
			SaveManager::Instance().SaveGame(e, SaveType::Manual);
			e.Quit();
		});

		PushMenu(std::move(menu));
	}

	void Engine::ShowLevelUpMenu()
	{
		auto menu = std::make_unique<Menu>();
		menu->SetAllowEscape(false); // Must choose stat upgrade

		menu->AddItem("Strength (+1 attack)", [](Engine& e) {
			auto* player = dynamic_cast<Player*>(e.GetPlayer());
			if (player && player->GetAttacker()) {
				player->GetAttacker()->IncreaseStrength(1);
				e.LogMessage("Your strength increases by 1!",
				             tcod::ColorRGB { 255, 100, 0 },
				             false);
			}

			// Always grant +4 HP on level up
			if (player && player->GetDestructible()) {
				player->GetDestructible()->IncreaseMaxHealth(4);
				e.LogMessage("Your health increases by 4 HP!",
				             tcod::ColorRGB { 0, 255, 0 },
				             false);
			}

			e.PopMenu();
		});

		menu->AddItem("Dexterity (+1 defense)", [](Engine& e) {
			auto* player = dynamic_cast<Player*>(e.GetPlayer());
			if (player && player->GetDestructible()) {
				player->GetDestructible()->IncreaseDexterity(1);
				e.LogMessage("Your dexterity increases by 1!",
				             tcod::ColorRGB { 100, 100, 255 },
				             false);
			}

			if (player && player->GetDestructible()) {
				player->GetDestructible()->IncreaseMaxHealth(4);
				e.LogMessage("Your health increases by 4 HP!",
				             tcod::ColorRGB { 0, 255, 0 },
				             false);
			}

			e.PopMenu();
		});

		menu->AddItem("Intelligence (+1 mana)", [](Engine& e) {
			auto* player = dynamic_cast<Player*>(e.GetPlayer());
			if (player && player->GetDestructible()) {
				player->GetDestructible()->IncreaseIntelligence(
				    1);
				e.LogMessage(
				    "Your intelligence increases by 1!",
				    tcod::ColorRGB { 138, 43, 226 }, false);
				e.LogMessage(
				    "Your maximum mana increases by 1!",
				    tcod::ColorRGB { 0, 100, 200 }, false);
			}

			if (player && player->GetDestructible()) {
				player->GetDestructible()->IncreaseMaxHealth(4);
				e.LogMessage("Your health increases by 4 HP!",
				             tcod::ColorRGB { 0, 255, 0 },
				             false);
			}

			e.PopMenu();
		});

		PushMenu(std::move(menu));
	}

	void Engine::ShowInventory()
	{
		auto menu = std::make_unique<Menu>();
		auto* player = dynamic_cast<Player*>(player_);
		if (!player) {
			return;
		}

		const auto& inventory = player->GetInventory();

		if (inventory.empty()) {
			menu->AddItem("(empty)",
			              [](Engine& e) { e.PopMenu(); });
		} else {
			for (size_t i = 0; i < inventory.size(); ++i) {
				const auto& item = inventory[i];
				std::string label = item->GetName();

				// Capture index for use/drop
				menu->AddItem(label, [i](Engine& e) {
					if (e.GetInventoryMode()
					    == InventoryMode::Drop) {
						auto* p = dynamic_cast<Player*>(
						    e.GetPlayer());
						if (p) {
							auto droppedItem =
							    p->ExtractFromInventory(
							        i);
							if (droppedItem) {
								e.SpawnEntity(
								    std::move(
								        droppedItem),
								    p->GetPos());
								e.LogMessage(
								    "You drop "
								    "the item.",
								    tcod::
								        ColorRGB {
								            200,
								            200,
								            200 },
								    false);
							}
						}
					} else {
						// Use item
						auto event = std::make_unique<
						    UseItemAction>(
						    e, *e.GetPlayer(), i);
						event->Execute();
					}
					e.PopMenu();
				});
			}
		}

		PushMenu(std::move(menu));
	}

	void Engine::ShowItemSelection(const std::vector<Entity*>& items)
	{
		itemSelectionList_ = items;

		auto menu = std::make_unique<Menu>();

		for (Entity* item : items) {
			menu->AddItem(item->GetName(), [item](Engine& e) {
				auto event = std::make_unique<PickupItemAction>(
				    e, *e.GetPlayer(), item);
				event->Execute();
				e.PopMenu();
			});
		}

		PushMenu(std::move(menu));
	}

	void Engine::GrantXpToPlayer(unsigned int xpAmount)
	{
		if (!player_ || !player_->GetDestructible()) {
			return;
		}

		auto* destructible = player_->GetDestructible();
		unsigned int oldXp = destructible->GetXp();

		destructible->AddXp(xpAmount);

		LogMessage(
		    "You gain " + std::to_string(xpAmount) + " experience!",
		    { 0, 255, 0 }, false);

		if (destructible->CheckLevelUp(oldXp, destructible->GetXp())) {
			unsigned int newLevel =
			    destructible->CalculateLevel(destructible->GetXp());
			LogMessage(
			    "Your battle skills grow stronger! You "
			    "reached level "
			        + std::to_string(newLevel) + "!",
			    { 255, 255, 0 }, false);
			ShowLevelUpMenu();
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

	Entity* Engine::GetClosestMonster(pos_t pos, float range) const
	{
		Entity* closest = nullptr;
		float bestDistance = 1E6f;

		for (const auto& entity : entities_) {
			if (entity->GetFaction() == Faction::MONSTER
			    && entity->GetDestructible()
			    && !entity->GetDestructible()->IsDead())

			{
				float distance =
				    entity->GetDistance(pos.x, pos.y);
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

	Engine::PlayerState Engine::SavePlayerState()
	{
		std::string name = player_ ? player_->GetName() : "player";
		AttackerComponent attacker = (player_ && player_->GetAttacker())
		                                 ? *player_->GetAttacker()
		                                 : AttackerComponent { 5 };
		DestructibleComponent destructible =
		    (player_ && player_->GetDestructible())
		        ? *player_->GetDestructible()
		        : DestructibleComponent { 1, 30, 30 };

		PlayerState state(name, attacker, destructible);

		if (player_) {
			if (auto* playerPtr = dynamic_cast<Player*>(player_)) {
				size_t invSize = playerPtr->GetInventorySize();
				for (size_t i = 0; i < invSize; ++i) {
					auto item =
					    playerPtr->ExtractFromInventory(0);
					if (item) {
						state.inventory.push_back(
						    std::move(item));
					}
				}
			}
		}

		return state;
	}

	void Engine::LoadLevelConfiguration(int dungeonLevel)
	{
		std::string levelConfigPath;

		if (dungeonLevel == 1) {
			levelConfigPath = "data/levels/dungeon_1.json";
		} else if (dungeonLevel == 2) {
			levelConfigPath = "data/levels/dungeon_2.json";
		} else {
			levelConfigPath = "data/levels/dungeon_2.json";
		}

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

		try {
			DynamicSpawnSystem::Instance().Clear();
			DynamicSpawnSystem::Instance().BuildSpawnTablesForLevel(
			    currentLevel_);
		} catch (const std::exception& e) {
			std::cerr << "[Engine] FATAL: Failed to build "
			             "spawn tables: "
			          << e.what() << std::endl;
			throw;
		}
	}

	void Engine::ClearCurrentLevel()
	{
		entities_.Clear();
		eventQueue_.clear();
		entitiesToRemove_.clear();
		player_ = nullptr;
		stairs_ = nullptr;
	}

	void Engine::PopulateLevelWithEntities()
	{
		GenerateMap(currentLevel_.generation.width,
		            currentLevel_.generation.height);
		map_->Update();

		auto rooms = map_->GetRooms();
		if (rooms.empty()) {
			std::cerr << "[Engine] FATAL: No rooms generated!"
			          << std::endl;
			return;
		}

		for (auto it = rooms.begin() + 1; it != rooms.end(); ++it) {
			entities_.PlaceItems(*it, currentLevel_.itemSpawning,
			                     currentLevel_.id);
			entities_.PlaceEntities(*it,
			                        currentLevel_.monsterSpawning,
			                        currentLevel_.id);
		}

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
	}

	void Engine::RestorePlayerWithState(PlayerState&& state, pos_t position)
	{
		auto playerEntity = std::make_unique<Player>(
		    position, state.name, true, state.attacker,
		    state.destructible,
		    IconRenderable { { 255, 255, 255 }, '@' }, Faction::PLAYER);

		for (auto& item : state.inventory) {
			playerEntity->AddToInventory(std::move(item));
		}

		player_ = entities_.Spawn(std::move(playerEntity)).get();
	}

	void Engine::RecreatePlayerUI()
	{
		auto& cfg = ConfigManager::Instance();

		healthBar_ = std::make_unique<HealthBar>(
		    cfg.GetHealthBarWidth(), cfg.GetHealthBarHeight(),
		    pos_t { cfg.GetHealthBarX(), cfg.GetHealthBarY() },
		    *player_);
	}

	pos_t Engine::CalculateWindowPosition(int width, int height,
	                                      bool center) const
	{
		if (center) {
			return pos_t { static_cast<int>(config_.width) / 2
				           - width / 2,
				       static_cast<int>(config_.height) / 2
				           - height / 2 };
		}
		return pos_t { 0, 0 };
	}

	void tutorial::Engine::NextLevel()
	{
		dungeonLevel_++;

		std::cout << "[Engine] Descending to dungeon level "
		          << dungeonLevel_ << std::endl;

		LogMessage(
		    "After a rare moment of peace, you descend deeper "
		    "into "
		    "the heart of the dungeon...",
		    { 255, 60, 60 }, false);

		PlayerState savedState = SavePlayerState();
		LoadLevelConfiguration(dungeonLevel_);
		ClearCurrentLevel();
		PopulateLevelWithEntities();

		auto rooms = map_->GetRooms();
		if (!rooms.empty()) {
			pos_t playerPos = rooms[0].GetCenter();
			RestorePlayerWithState(std::move(savedState),
			                       playerPos);
			RecreatePlayerUI();
		}

		ComputeFOV();
		map_->Update();

		LogMessage("Welcome to dungeon level "
		               + std::to_string(dungeonLevel_) + "!",
		           { 255, 255, 0 }, false);

		gameState_ = GameState::MainGame;
		eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
	}

	bool Engine::PickATile(pos_t* pos, float maxRange,
	                       std::function<bool(pos_t)> validator,
	                       TargetingType targetingType, float radius)
	{
		// Remember previous game state to restore later
		GameState previousGameState = gameState_;

		// Enter targeting mode (blocks menu/other UI)
		gameState_ = GameState::MainGame;

		// Render current game state before targeting
		this->Render();

		// Create targeting cursor (handles all targeting logic)
		TargetingCursor cursor(*this, maxRange, targetingType, radius);

		// Let cursor handle all input and selection with
		// validator
		bool result = cursor.SelectTile(pos, validator);

		// Restore game state
		gameState_ = previousGameState;

		// Cursor destructor automatically restores console
		// state
		return result;
	}

	Entity* Engine::GetActor(pos_t pos) const
	{
		for (const auto& entity : entities_) {
			if (entity->GetPos() == pos && entity->GetDestructible()
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
		// Don't deal damage to corpses - they're already dead
		if (target.IsCorpse()) {
			return;
		}

		target.GetDestructible()->TakeDamage(damage);

		if (target.GetDestructible()->IsDead()) {
			// Create and execute DieAction to handle XP,
			// messages, etc.
			std::unique_ptr<Event> dieAction =
			    std::make_unique<DieAction>(*this, target);
			dieAction->Execute();
		}
	}

	void Engine::RenderGameBackground(TCOD_Console* console)
	{
		map_->Render(console);

		// Render all entities in FOV
		for (const auto& entity : entities_) {
			const auto pos = entity->GetPos();
			if (map_->IsInFov(pos)) {
				const auto& renderable =
				    entity->GetRenderable();
				renderable->Render(console, pos);
			}
		}

		// Render health bar if player exists
		if (player_ && healthBar_) {
			healthBar_->Render(console);
		}

		messageLogWindow_->Render(console);
	}

	void Engine::Render()
	{
		TCOD_console_clear(console_);

		if (gameState_ == GameState::MessageHistory) {
			messageHistoryWindow_->Render(console_);
		} else if (gameState_ == GameState::InMenu) {
			// Show game in background if player exists
			// (pause/inventory/levelup) Don't show for
			// start menu (player_ is nullptr)
			if (player_) {
				RenderGameBackground(console_);
			} else {
				// Main menu - render opaque black background
				tcod::ColorRGB black { 0, 0, 0 };
				for (int y = 0; y < config_.height; ++y) {
					for (int x = 0; x < config_.width; ++x) {
						TCOD_console_put_rgb(
						    console_, x, y, ' ', &black,
						    &black, TCOD_BKGND_SET);
					}
				}
			}

			// Render topmost menu centered
			if (!menuStack_.empty()) {
				int x = 10;
				int y = 10;
				menuStack_.back()->Render(console_, x, y);
			}
		} else {
			// GameState::MainGame
			RenderGameBackground(console_);
			messageLogWindow_->RenderMouseLook(console_, *this);
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
				// Override the generic name with
				// specific corpse name
				corpse->SetName(corpseName);

				// Set corpse priority to -1 so it
				// renders below any items at this
				// position (Items have default priority
				// 0 or higher within the ITEMS layer)
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
