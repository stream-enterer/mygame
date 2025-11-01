#include "Engine.hpp"

#include "CharacterCreationWindow.hpp"
#include "Colors.hpp"
#include "DynamicSpawnSystem.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "EventHandler.hpp"
#include "HealthBar.hpp"
#include "InventoryWindow.hpp"
#include "ItemSelectionWindow.hpp"
#include "LevelConfig.hpp"
#include "LocaleManager.hpp"
#include "Map.hpp"
#include "MapGenerator.hpp"
#include "MenuWindow.hpp"
#include "MessageHistoryWindow.hpp"
#include "MessageLogWindow.hpp"
#include "SaveManager.hpp"
#include "SpellMenuWindow.hpp"
#include "SpellRegistry.hpp"
#include "SpellcasterComponent.hpp"

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
	      rootConsole_(nullptr),
	      gameConsole_(nullptr),
	      window_(nullptr),
	      windowState_(StartMenu),
	      gameOver_(false),
	      running_(true),
	      mousePos_ { 0, 0 },
	      inventoryMode_(InventoryMode::Use)
	{
		// Create root console for full window
		rootConsole_ = TCOD_console_new(config.width, config.height);
		if (!rootConsole_) {
			SDL_Quit();
			throw std::runtime_error(
			    "Failed to create root console");
		}

		// Create game console for map view only (separate from UI)
		auto& cfg = ConfigManager::Instance();
		int gameViewHeight = config.height - cfg.GetMapHeightOffset();
		gameConsole_ = TCOD_console_new(config.width, gameViewHeight);
		if (!gameConsole_) {
			TCOD_console_delete(rootConsole_);
			SDL_Quit();
			throw std::runtime_error(
			    "Failed to create game console");
		}

		// Load BDF font if path is provided
		if (!config.fontPath.empty()) {
			try {
				tileset_ = tcod::load_bdf(config.fontPath);
			} catch (const std::exception& e) {
				TCOD_console_delete(gameConsole_);
				TCOD_console_delete(rootConsole_);
				SDL_Quit();
				throw std::runtime_error(
				    std::string("Failed to load BDF font: ")
				    + e.what());
			}
		}

		TCOD_ContextParams params = {};
		params.tcod_version = TCOD_COMPILEDVERSION;
		params.console = rootConsole_;
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
			TCOD_console_delete(gameConsole_);
			TCOD_console_delete(rootConsole_);
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
		if (gameConsole_) {
			TCOD_console_delete(gameConsole_);
		}
		if (rootConsole_) {
			TCOD_console_delete(rootConsole_);
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

		// Regenerate MP for all entities with spellcaster component
		for (auto& entity : entities_) {
			if (entity->GetSpellcaster()
			    && entity->GetDestructible()) {
				entity->GetDestructible()->RegenerateMp(1);
			}
		}

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

			// Load spells
			SpellRegistry::Instance().Clear();
			SpellRegistry::Instance().LoadFromDirectory(
			    "data/spells");

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

		// Give player starting spells and MP
		if (player_) {
			auto* destructible = player_->GetDestructible();
			if (destructible) {
				// Set player's intelligence to 20 for testing
				// (gives 20 MP)
				destructible->IncreaseIntelligence(
				    19); // +19 to base of 1
			}

			// Create spellcaster component with starting spells
			auto spellcaster =
			    std::make_unique<SpellcasterComponent>();
			spellcaster->AddSpell("fireball");
			spellcaster->AddSpell("lightning_bolt");
			spellcaster->AddSpell("chain_lightning");
			spellcaster->AddSpell("confusion");

			// Attach to player using SetSpellcaster
			if (auto* basePlayer =
			        dynamic_cast<BaseEntity*>(player_)) {
				basePlayer->SetSpellcaster(
				    std::move(spellcaster));
			}
		}

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
		pos_t invPos = CalculateWindowPosition(
		    invWidth, invHeight, cfg.GetInventoryCenterOnScreen());

		inventoryWindow_ = std::make_unique<InventoryWindow>(
		    invWidth, invHeight, invPos, *player_);

		this->ComputeFOV();

		auto welcomeMsg =
		    LocaleManager::Instance().GetMessage("game.welcome");
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

	void Engine::ShowSpellMenu()
	{
		if (windowState_ != SpellMenu) {
			eventHandler_ =
			    std::make_unique<SpellMenuEventHandler>(*this);
			windowState_ = SpellMenu;

			// Create spell menu window
			auto& cfg = ConfigManager::Instance();
			int width = cfg.GetInventoryWindowWidth();
			int height = cfg.GetInventoryWindowHeight();
			pos_t pos = CalculateWindowPosition(
			    width, height, cfg.GetInventoryCenterOnScreen());

			spellMenuWindow_ = std::make_unique<SpellMenuWindow>(
			    width, height, pos, *player_);
		}
	}

	void Engine::ShowItemSelection(const std::vector<Entity*>& items)
	{
		if (windowState_ != ItemSelection) {
			itemSelectionList_ = items;

			auto& cfg = ConfigManager::Instance();
			int width = cfg.GetInventoryWindowWidth();
			int height = cfg.GetInventoryWindowHeight();
			pos_t pos = CalculateWindowPosition(
			    width, height, cfg.GetInventoryCenterOnScreen());

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
			menuWindow_->AddItem(MenuAction::LevelUpStrength,
			                     "Strength (+1 attack)");
			menuWindow_->AddItem(MenuAction::LevelUpDexterity,
			                     "Dexterity (+1 defense)");
			menuWindow_->AddItem(MenuAction::LevelUpIntelligence,
			                     "Intelligence (+1 mana)");

			eventHandler_ =
			    std::make_unique<LevelUpMenuEventHandler>(*this);
			windowState_ = LevelUpMenu;
		}
	}

	void Engine::ShowStartMenu()
	{
		// Create full-screen menu window with border at screen edges
		int width = static_cast<int>(config_.width);
		int height = static_cast<int>(config_.height);
		pos_t pos { 0, 0 };

		menuWindow_ = std::make_unique<MenuWindow>(
		    width, height, pos, "My Game",
		    true); // fullScreenBorder = true

		// Set game logo stub
		menuWindow_->SetGameLogoStub("[GameLogo]");

		// Build start menu options
		menuWindow_->Clear();

		// Only show "Continue" if a save file exists - make it first
		// option
		if (SaveManager::Instance().HasSave()) {
			menuWindow_->AddItem(MenuAction::Continue, "Continue");
		}

		menuWindow_->AddItem(MenuAction::NewGame, "New Game");
		menuWindow_->AddItem(MenuAction::Quit, "Exit");

		eventHandler_ = std::make_unique<StartMenuEventHandler>(*this);
		windowState_ = StartMenu;
	}

	void Engine::ShowCharacterCreation()
	{
		// Clear any previous menu window (e.g., confirmation dialog)
		menuWindow_.reset();

		// Create full-screen character creation window
		int width = static_cast<int>(config_.width);
		int height = static_cast<int>(config_.height);
		pos_t pos { 0, 0 };

		characterCreationWindow_ =
		    std::make_unique<CharacterCreationWindow>(width, height,
		                                              pos);

		eventHandler_ =
		    std::make_unique<CharacterCreationEventHandler>(*this);
		windowState_ = CharacterCreation;
	}

	void Engine::ShowNewGameConfirmation()
	{
		// Create confirmation dialog centered on screen
		int width = 50;
		int height = 15;
		pos_t pos { static_cast<int>(config_.width) / 2 - width / 2,
			    static_cast<int>(config_.height) / 2 - height / 2 };

		menuWindow_ = std::make_unique<MenuWindow>(
		    width, height, pos, "Abandon Current Save?");

		// Build confirmation options
		menuWindow_->Clear();
		menuWindow_->AddItem(MenuAction::ConfirmNo,
		                     "No - Return to Menu");
		menuWindow_->AddItem(MenuAction::ConfirmYes,
		                     "Yes - Start New Game");

		// Reuse CharacterCreationEventHandler since it has same input
		// handling (UP/DOWN/ENTER/SPACE/ESC)
		eventHandler_ =
		    std::make_unique<CharacterCreationEventHandler>(*this);
		windowState_ = NewGameConfirmation;
	}

	void Engine::MenuNavigateUp()
	{
		if (menuWindow_) {
			menuWindow_->SelectPrevious();
		}

		if (characterCreationWindow_) {
			characterCreationWindow_->SelectPrevious();
		}
	}

	void Engine::MenuNavigateDown()
	{
		if (menuWindow_) {
			menuWindow_->SelectNext();
		}

		if (characterCreationWindow_) {
			characterCreationWindow_->SelectNext();
		}
	}

	void Engine::MenuNavigateLeft()
	{
		if (characterCreationWindow_) {
			characterCreationWindow_->SelectPreviousTab();
		}
	}

	void Engine::MenuNavigateRight()
	{
		if (characterCreationWindow_) {
			characterCreationWindow_->SelectNextTab();
		}
	}

	void Engine::MenuSelectByLetter(char letter)
	{
		if (menuWindow_) {
			menuWindow_->SelectByLetter(letter);
		}

		if (characterCreationWindow_) {
			characterCreationWindow_->SelectByLetter(letter);
		}
	}

	void Engine::MenuIncrementStat()
	{
		if (characterCreationWindow_) {
			characterCreationWindow_->IncrementStat();
		}
	}

	void Engine::MenuDecrementStat()
	{
		if (characterCreationWindow_) {
			characterCreationWindow_->DecrementStat();
		}
	}

	void Engine::HandleCharacterCreationConfirm(MenuAction action)
	{
		switch (action) {
			case MenuAction::ConfirmYes:
				if (characterCreationWindow_) {
					// Get selected class index
					characterCreation_.selectedClass =
					    characterCreationWindow_
					        ->GetSelectedClassIndex();

					// Clear the windows
					characterCreationWindow_.reset();
					menuWindow_.reset();

					// Start new game
					NewGame();
					ReturnToMainGame();
				}
				break;

			case MenuAction::ConfirmNo:
				// Return to character creation
				menuWindow_.reset();
				windowState_ = CharacterCreation;
				break;

			case MenuAction::None:
			default:
				break;
		}
	}

	void Engine::HandleStartMenuConfirm(MenuAction action)
	{
		switch (action) {
			case MenuAction::NewGame:
				// Check if save file exists - if so, show
				// confirmation
				if (SaveManager::Instance().HasSave()) {
					ShowNewGameConfirmation();
				} else {
					ShowCharacterCreation();
				}
				break;

			case MenuAction::Continue:
				if (SaveManager::Instance().LoadGame(*this)) {
					ReturnToMainGame();
				} else {
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
	}

	void Engine::HandlePauseMenuConfirm(MenuAction action)
	{
		switch (action) {
			case MenuAction::Continue:
				ReturnToMainGame();
				break;

			case MenuAction::SaveAndQuit:
				SaveManager::Instance().SaveGame(
				    *this, SaveType::Manual);
				ShowStartMenu();
				break;

			case MenuAction::None:
			default:
				break;
		}
	}

	void Engine::HandleLevelUpConfirm(MenuAction action)
	{
		if (!player_ || !player_->GetDestructible()) {
			ReturnToMainGame();
			return;
		}

		auto* destructible = player_->GetDestructible();
		auto* attacker = player_->GetAttacker();

		// Every level up grants +4 HP
		destructible->IncreaseMaxHealth(4);
		LogMessage("Your health increases by 4 HP!", { 0, 255, 0 },
		           false);

		switch (action) {
			case MenuAction::LevelUpStrength:
				if (attacker) {
					attacker->IncreaseStrength(1);
					LogMessage(
					    "Your strength increases by 1!",
					    { 255, 100, 0 }, false);
				}
				break;

			case MenuAction::LevelUpDexterity:
				destructible->IncreaseDexterity(1);
				LogMessage("Your dexterity increases by 1!",
				           { 100, 100, 255 }, false);
				break;

			case MenuAction::LevelUpIntelligence:
				destructible->IncreaseIntelligence(1);
				LogMessage("Your intelligence increases by 1!",
				           { 138, 43, 226 }, false);
				LogMessage("Your maximum mana increases by 1!",
				           { 0, 100, 200 }, false);
				break;

			case MenuAction::None:
			default:
				break;
		}

		ReturnToMainGame();
	}

	void Engine::HandleNewGameConfirmation(MenuAction action)
	{
		switch (action) {
			case MenuAction::ConfirmYes:
				// User confirmed - delete save and start new
				// game
				SaveManager::Instance().DeleteSave();
				ShowCharacterCreation();
				break;

			case MenuAction::ConfirmNo:
				// User canceled - return to start menu
				ShowStartMenu();
				break;

			case MenuAction::None:
			default:
				break;
		}
	}

	void Engine::MenuConfirm()
	{
		if (windowState_ == CharacterCreation) {
			if (characterCreationWindow_) {
				auto tab =
				    characterCreationWindow_->GetCurrentTab();
				if (tab == CreationTab::Confirm) {
					// Show confirmation dialog
					if (characterCreationWindow_
					        ->IsReadyToConfirm()) {
						// Create confirmation menu
						int width = 50;
						int height = 15;
						pos_t pos { static_cast<int>(
							        config_.width)
							            / 2
							        - width / 2,
							    static_cast<int>(
							        config_.height)
							            / 2
							        - height / 2 };

						menuWindow_ = std::make_unique<
						    MenuWindow>(
						    width, height, pos,
						    "Are you sure?");
						menuWindow_->Clear();
						menuWindow_->AddItem(
						    MenuAction::ConfirmYes,
						    "Yes");
						menuWindow_->AddItem(
						    MenuAction::ConfirmNo,
						    "No");
						windowState_ =
						    NewGameConfirmation;
					}
				} else {
					// Confirm selection in current tab
					characterCreationWindow_
					    ->ConfirmSelection();
				}
			}
			return;
		}

		if (!menuWindow_) {
			return;
		}

		MenuAction action = menuWindow_->GetSelectedAction();

		if (windowState_ == StartMenu) {
			HandleStartMenuConfirm(action);
		} else if (windowState_ == PauseMenu) {
			HandlePauseMenuConfirm(action);
		} else if (windowState_ == LevelUpMenu) {
			HandleLevelUpConfirm(action);
		} else if (windowState_ == NewGameConfirmation) {
			// Check if we came from character creation
			if (characterCreationWindow_) {
				HandleCharacterCreationConfirm(action);
			} else {
				HandleNewGameConfirmation(action);
			}
		}
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
			std::cerr
			    << "[Engine] FATAL: Failed to build spawn tables: "
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

		int invWidth = cfg.GetInventoryWindowWidth();
		int invHeight = cfg.GetInventoryWindowHeight();
		pos_t invPos = CalculateWindowPosition(
		    invWidth, invHeight, cfg.GetInventoryCenterOnScreen());

		inventoryWindow_ = std::make_unique<InventoryWindow>(
		    invWidth, invHeight, invPos, *player_);
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
		    "After a rare moment of peace, you descend deeper into "
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

		windowState_ = MainGame;
		eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
	}

	bool Engine::PickATile(pos_t* pos, float maxRange,
	                       std::function<bool(pos_t)> validator,
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
		bool result = cursor.SelectTile(pos, validator);

		// Restore window state
		windowState_ = previousWindowState;

		// Cursor destructor automatically restores console state
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
			// Create and execute DieAction to handle XP, messages,
			// etc.
			std::unique_ptr<Event> dieAction =
			    std::make_unique<DieAction>(*this, target);
			dieAction->Execute();
		}
	}

	void Engine::RenderGame()
	{
		// Clear and render ONLY the game world (map + entities)
		TCOD_console_clear(gameConsole_);

		map_->Render(gameConsole_);

		// Render all entities in FOV
		for (const auto& entity : entities_) {
			const auto pos = entity->GetPos();
			if (map_->IsInFov(pos)) {
				const auto& renderable =
				    entity->GetRenderable();
				renderable->Render(gameConsole_, pos);
			}
		}
	}

	void Engine::RenderUI(TCOD_Console* targetConsole)
	{
		// Render ONLY UI elements (health bar, message log, mouse look)
		if (healthBar_) {
			healthBar_->Render(targetConsole);
		}

		if (messageLogWindow_) {
			messageLogWindow_->Render(targetConsole);
		}
	}

	void Engine::RenderGameBackground(TCOD_Console* console)
	{
		// Composite layers: Game + UI
		RenderGame();

		// Blit game console to target
		TCOD_console_blit(gameConsole_, 0, 0,
		                  TCOD_console_get_width(gameConsole_),
		                  TCOD_console_get_height(gameConsole_),
		                  console, 0, 0, 1.0f, 1.0f);

		// Render UI on top
		RenderUI(console);
	}

	void Engine::Render()
	{
		TCOD_console_clear(rootConsole_);

		// Determine which layers to render based on window state
		bool renderGame = false;
		bool renderUI = false;
		bool renderOverlay = false;

		switch (windowState_) {
			case StartMenu:
			case CharacterCreation:
			case NewGameConfirmation:
			case MessageHistory:
				// Full-screen menus: no game/UI underneath
				renderOverlay = true;
				break;

			case MainGame:
				// Just game + UI
				renderGame = true;
				renderUI = true;
				break;

			case Inventory:
			case SpellMenu:
			case ItemSelection:
			case PauseMenu:
			case LevelUpMenu:
				// Game + UI + Overlay menu on top
				renderGame = true;
				renderUI = true;
				renderOverlay = true;
				break;
		}

		// Layer 1: Game world (map + entities)
		if (renderGame) {
			RenderGame();
			TCOD_console_blit(gameConsole_, 0, 0,
			                  TCOD_console_get_width(gameConsole_),
			                  TCOD_console_get_height(gameConsole_),
			                  rootConsole_, 0, 0, 1.0f, 1.0f);
		}

		// Layer 2: UI panels (health, message log, mouse look)
		if (renderUI) {
			RenderUI(rootConsole_);
			if (windowState_ == MainGame) {
				messageLogWindow_->RenderMouseLook(rootConsole_,
				                                   *this);
			}
		}

		// Layer 3: Overlay menus/windows
		if (renderOverlay) {
			switch (windowState_) {
				case StartMenu:
					if (menuWindow_) {
						menuWindow_->Render(
						    rootConsole_);
					}
					break;

				case CharacterCreation:
					if (characterCreationWindow_) {
						characterCreationWindow_
						    ->Render(rootConsole_);
					}
					if (menuWindow_) {
						menuWindow_->Render(
						    rootConsole_);
					}
					break;

				case NewGameConfirmation:
					if (menuWindow_) {
						menuWindow_->Render(
						    rootConsole_);
					}
					break;

				case MessageHistory:
					messageHistoryWindow_->Render(
					    rootConsole_);
					break;

				case Inventory:
					inventoryWindow_->Render(rootConsole_);
					break;

				case SpellMenu:
					spellMenuWindow_->Render(rootConsole_);
					break;

				case ItemSelection:
					itemSelectionWindow_->Render(
					    rootConsole_);
					break;

				case PauseMenu:
				case LevelUpMenu:
					if (menuWindow_) {
						menuWindow_->Render(
						    rootConsole_);
					}
					break;

				default:
					break;
			}
		}

		TCOD_context_present(context_, rootConsole_, &viewportOptions_);
	}

	void Engine::RenderGameUI(TCOD_Console* targetConsole) const
	{
		// Render health bar (always show, even if player is dead)
		if (healthBar_) {
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
				// corpse name
				corpse->SetName(corpseName);

				// Set corpse priority to -1 so it renders below
				// any items at this position (Items have
				// default priority 0 or higher within the ITEMS
				// layer)
				corpse->SetRenderPriority(-1);
				SpawnEntity(std::move(corpse), corpsePos);
			}

			// SPECIAL CASE: Don't remove player entity to keep
			// HealthBar reference valid Only nullify the pointer so
			// game logic knows player is dead
			if (entity == player_) {
				player_ = nullptr;
			} else {
				// Remove non-player entities normally
				RemoveEntity(entity);
			}
		}

		entitiesToRemove_.clear();
	}

} // namespace tutorial
