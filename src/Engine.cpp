#include "Engine.hpp"

#include "Colors.hpp"
#include "DynamicSpawnSystem.hpp"
#include "Entity.hpp"
#include "Event.hpp"
#include "EventHandler.hpp"
#include "HealthBar.hpp"
#include "InventoryWindow.hpp"
#include "LevelConfig.hpp"
#include "Map.hpp"
#include "MapGenerator.hpp"
#include "MessageHistoryWindow.hpp"
#include "MessageLogWindow.hpp"
#include "StringTable.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace tutorial
{
    // Public methods
    Engine::Engine(const Configuration& config) :
        config_(config),
        eventHandler_(std::make_unique<MainGameEventHandler>(*this)),
        map_(nullptr), // Initialize to nullptr, create in NewGame()
        messageHistoryWindow_(std::make_unique<MessageHistoryWindow>(
            config.width, config.height, pos_t{ 0, 0 }, messageLog_)),
        messageLogWindow_(
            nullptr), // Initialize to nullptr, create in NewGame()
        player_(nullptr),
        healthBar_(nullptr),
        context_(nullptr),
        console_(nullptr),
        window_(nullptr),
        windowState_(MainGame),
        gameOver_(false),
        running_(true),
        mousePos_{ 0, 0 },
        inventoryMode_(InventoryMode::Use)
    {
        // Create console - this is the display buffer we'll draw to
        console_ = TCOD_console_new(config.width, config.height);
        if (!console_)
        {
            SDL_Quit();
            throw std::runtime_error("Failed to create console");
        }

        // Create context parameters - this sets up how the console will be
        // displayed
        TCOD_ContextParams params = {};
        params.tcod_version =
            TCOD_COMPILEDVERSION;  // Tell libtcod which version we compiled
                                   // against
        params.console = console_; // The console buffer to display
        params.window_title = config.title.c_str();     // Window title
        params.sdl_window_flags = SDL_WINDOW_RESIZABLE; // Make window resizable
        params.vsync = 1;                               // Enable vsync
        params.argc = 0;
        params.argv = nullptr;

        // Actually create the context - this creates the SDL window and
        // rendering setup
        if (TCOD_context_new(&params, &context_) != TCOD_E_OK)
        {
            TCOD_console_delete(console_);
            SDL_Quit();
            throw std::runtime_error("Failed to create context");
        }

        // Get the SDL window from the context so we can work with it if needed
        window_ = TCOD_context_get_sdl_window(context_);

        this->NewGame();
    }

    Engine::~Engine()
    {
        // Clean up in reverse order of creation
        if (context_)
        {
            TCOD_context_delete(context_);
        }
        if (console_)
        {
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
        map_->Update();
    }

    std::unique_ptr<Command> Engine::GetInput()
    {
        return eventHandler_->Dispatch();
    }

    void Engine::HandleDeathEvent(Entity& entity)
    {
        if (this->IsPlayer(entity))
        {
            eventHandler_ = std::make_unique<GameOverEventHandler>(*this);
            eventQueue_.clear();
            gameOver_ = true;

            entity.Die(); // Keep player as dead entity (special case for game
                          // over screen)
            entities_.SortByRenderLayer();
            return;
        }

        // For non-player entities: mark for deferred removal
        // Don't remove immediately - event system still has references!
        entitiesToRemove_.push_back(&entity);
    }

    void Engine::HandleEvents()
    {
        while (!eventQueue_.empty())
        {
            auto event = std::move(eventQueue_.front());
            eventQueue_.pop_front();
            event->Execute();
        }

        eventQueue_.clear();

        // Process any entities that died during event processing
        ProcessDeferredRemovals();
    }

    void Engine::LogMessage(const std::string& text, tcod::ColorRGB color,
                            bool stack)
    {
        messageLog_.AddMessage(text, color, stack);
    }

    void Engine::NewGame()
    {
        // Load configuration files (first time only)
        static bool configLoaded = false;
        if (!configLoaded)
        {
            ConfigManager::Instance().LoadAll();
            configLoaded = true;
        }

        // Create map if not yet created (first NewGame call)
        if (!map_)
        {
            auto& cfg = ConfigManager::Instance();
            map_ = std::make_unique<Map>(
                config_.width, config_.height - cfg.GetMapHeightOffset());
        }

        // Create message log window if not yet created
        if (!messageLogWindow_)
        {
            auto& cfg = ConfigManager::Instance();
            messageLogWindow_ = std::make_unique<MessageLogWindow>(
                cfg.GetMessageLogWidth(), cfg.GetMessageLogHeight(),
                pos_t{ cfg.GetMessageLogX(), cfg.GetMessageLogY() },
                messageLog_);
        }

        // Load current level configuration
        currentLevel_ = LevelConfig::LoadFromFile("data/levels/dungeon_1.json");

        // Load entity templates from JSON
        try
        {
            TemplateRegistry::Instance()
                .Clear(); // Clear any existing templates
            TemplateRegistry::Instance().LoadFromDirectory("data/entities");
            std::cout << "[Engine] Loaded "
                      << TemplateRegistry::Instance().GetAllIds().size()
                      << " entity templates" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Engine] FATAL: Failed to load entity templates: "
                      << e.what() << std::endl;
            throw; // Re-throw, can't continue without templates
        }

        // Build spawn tables dynamically from level config
        try
        {
            DynamicSpawnSystem::Instance().Clear();
            DynamicSpawnSystem::Instance().BuildSpawnTablesForLevel(
                currentLevel_);
            DynamicSpawnSystem::Instance().ValidateSpawnData();
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Engine] FATAL: Failed to build spawn tables: "
                      << e.what() << std::endl;
            throw;
        }

        // Clear the entities and message log
        entities_.Clear();
        messageLog_.Clear();
        eventQueue_.clear();

        // Generate map using level config parameters
        this->GenerateMap(currentLevel_.generation.width,
                          currentLevel_.generation.height);

        auto rooms = map_->GetRooms();

        // Place items FIRST (so they render on bottom)
        for (auto it = rooms.begin() + 1; it != rooms.end(); ++it)
        {
            entities_.PlaceItems(*it, currentLevel_.itemSpawning,
                                 currentLevel_.id);
        }

        // Place monsters AFTER items (so they render on top)
        for (auto it = rooms.begin() + 1; it != rooms.end(); ++it)
        {
            entities_.PlaceEntities(*it, currentLevel_.monsterSpawning,
                                    currentLevel_.id);
        }

        // Create player and add them to entity list
        auto playerEntity =
            TemplateRegistry::Instance().Create("player", rooms[0].GetCenter());
        player_ = entities_.Spawn(std::move(playerEntity)).get();

        // Create health bar from config
        auto& cfg = ConfigManager::Instance();
        healthBar_ = std::make_unique<HealthBar>(
            cfg.GetHealthBarWidth(), cfg.GetHealthBarHeight(),
            pos_t{ cfg.GetHealthBarX(), cfg.GetHealthBarY() }, *player_);

        // Create inventory window from config
        int invWidth = cfg.GetInventoryWindowWidth();
        int invHeight = cfg.GetInventoryWindowHeight();
        pos_t invPos;

        if (cfg.GetInventoryCenterOnScreen())
        {
            invPos = pos_t{ config_.width / 2 - invWidth / 2,
                            config_.height / 2 - invHeight / 2 };
        }
        else
        {
            invPos =
                pos_t{ 0,
                       0 }; // Could add explicit position in config if needed
        }

        inventoryWindow_ = std::make_unique<InventoryWindow>(
            invWidth, invHeight, invPos, *player_);

        this->ComputeFOV();

        // Show welcome message
        auto welcomeMsg = StringTable::Instance().GetMessage("game.welcome");
        messageLog_.AddMessage(welcomeMsg.text, welcomeMsg.color,
                               welcomeMsg.stack);

        windowState_ = MainGame;

        eventHandler_ = std::make_unique<MainGameEventHandler>(*this);

        gameOver_ = false;
    }

    void Engine::ReturnToMainGame()
    {
        if (windowState_ != MainGame)
        {
            eventHandler_ = std::make_unique<MainGameEventHandler>(*this);
            windowState_ = MainGame;
        }
    }

    void Engine::ShowMessageHistory()
    {
        if (windowState_ != MessageHistory)
        {
            eventHandler_ = std::make_unique<MessageHistoryEventHandler>(*this);
            windowState_ = MessageHistory;
        }
    }

    void Engine::ShowInventory()
    {
        if (windowState_ != Inventory)
        {
            eventHandler_ = std::make_unique<InventoryEventHandler>(*this);
            windowState_ = Inventory;

            // Apply the current inventory mode to the handler
            if (auto* invHandler =
                    dynamic_cast<InventoryEventHandler*>(eventHandler_.get()))
            {
                invHandler->SetMode(inventoryMode_);

                // Set window title based on mode
                if (inventoryMode_ == InventoryMode::Drop)
                {
                    inventoryWindow_->SetTitle("Drop which item?");
                }
                else
                {
                    inventoryWindow_->SetTitle("Inventory");
                }
            }

            // Reset mode to Use for next time
            inventoryMode_ = InventoryMode::Use;
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

    Entity* Engine::SpawnEntity(std::unique_ptr<Entity> entity, pos_t pos,
                                bool atFront)
    {
        entity->SetPos(pos);
        if (atFront)
        {
            return entities_.SpawnAtFront(std::move(entity), pos).get();
        }
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

    const EntityManager& Engine::GetEntities() const
    {
        return entities_;
    }

    bool Engine::IsBlocker(pos_t pos) const
    {
        if (this->GetBlockingEntity(pos))
        {
            return true;
        }

        if (this->IsWall(pos))
        {
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
        // Check if window is still open and we haven't quit
        return running_ && window_ != nullptr;
    }

    bool Engine::IsValid(Entity& entity) const
    {
        auto it = std::find_if(entities_.begin(), entities_.end(),
                               [&entity](const auto& it)
                               { return (it.get() == &entity); });

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

        for (const auto& entity : entities_)
        {
            // Only target monsters (not player, items, or neutral)
            if (entity->GetFaction() == Faction::MONSTER
                && entity->GetDestructible()
                && !entity->GetDestructible()->IsDead())

            {
                float distance = entity->GetDistance(x, y);
                if (distance < bestDistance
                    && (distance <= range || range == 0.0f))
                {
                    bestDistance = distance;
                    closest = entity.get();
                }
            }
        }

        return closest;
    }

    bool Engine::PickATile(int* x, int* y, float maxRange)
    {
        SDL_Event sdlEvent;
        pos_t lastMousePos{ -1, -1 };

        // Render the base game state ONCE at the start
        this->Render();

        // Store the original background colors before we modify them
        std::vector<tcod::ColorRGB> originalColors(map_->GetWidth()
                                                   * map_->GetHeight());
        for (int cx = 0; cx < map_->GetWidth(); cx++)
        {
            for (int cy = 0; cy < map_->GetHeight(); cy++)
            {
                TCOD_color_t tcodCol =
                    TCOD_console_get_char_background(console_, cx, cy);
                originalColors[cx + cy * map_->GetWidth()] =
                    tcod::ColorRGB{ tcodCol.r, tcodCol.g, tcodCol.b };
            }
        }

        // Apply range highlighting ONCE
        for (int cx = 0; cx < map_->GetWidth(); cx++)
        {
            for (int cy = 0; cy < map_->GetHeight(); cy++)
            {
                if (map_->IsInFov(pos_t{ cx, cy })
                    && (maxRange == 0.0f
                        || player_->GetDistance(cx, cy) <= maxRange))
                {
                    // Brighten the original color
                    tcod::ColorRGB col =
                        originalColors[cx + cy * map_->GetWidth()];
                    col.r = std::min(255, static_cast<int>(col.r * 1.2f));
                    col.g = std::min(255, static_cast<int>(col.g * 1.2f));
                    col.b = std::min(255, static_cast<int>(col.b * 1.2f));

                    TCOD_console_set_char_background(console_, cx, cy, col,
                                                     TCOD_BKGND_SET);
                }
            }
        }

        // Highlight initial mouse position
        if (map_->IsInFov(mousePos_)
            && (maxRange == 0.0f
                || player_->GetDistance(mousePos_.x, mousePos_.y) <= maxRange))
        {
            TCOD_console_set_char_background(console_, mousePos_.x, mousePos_.y,
                                             color::white, TCOD_BKGND_SET);
        }
        lastMousePos = mousePos_;

        // Present the initial frame with highlights
        TCOD_context_present(context_, console_, nullptr);

        while (running_)
        {
            // Handle mouse/keyboard events
            while (SDL_PollEvent(&sdlEvent))
            {
                if (sdlEvent.type == SDL_EVENT_QUIT)
                {
                    // Allow window manager quit
                    this->Quit();
                    return false;
                }

                if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION)
                {
                    // Update mouse position
                    SDL_Window* window = TCOD_context_get_sdl_window(context_);
                    int windowWidth, windowHeight;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

                    int tileX =
                        (sdlEvent.motion.x * config_.width) / windowWidth;
                    int tileY =
                        (sdlEvent.motion.y * config_.height) / windowHeight;

                    pos_t newMousePos{ tileX, tileY };

                    // Only update if mouse actually moved to a different tile
                    if (newMousePos != lastMousePos)
                    {
                        // Restore the old mouse tile to range-highlighted color
                        if (lastMousePos.x >= 0 && lastMousePos.y >= 0
                            && map_->IsInFov(lastMousePos)
                            && (maxRange == 0.0f
                                || player_->GetDistance(lastMousePos.x,
                                                        lastMousePos.y)
                                       <= maxRange))
                        {
                            // Restore from original color, then brighten
                            tcod::ColorRGB col =
                                originalColors[lastMousePos.x
                                               + lastMousePos.y
                                                     * map_->GetWidth()];
                            col.r =
                                std::min(255, static_cast<int>(col.r * 1.2f));
                            col.g =
                                std::min(255, static_cast<int>(col.g * 1.2f));
                            col.b =
                                std::min(255, static_cast<int>(col.b * 1.2f));
                            TCOD_console_set_char_background(
                                console_, lastMousePos.x, lastMousePos.y, col,
                                TCOD_BKGND_SET);
                        }

                        mousePos_ = newMousePos;
                        lastMousePos = newMousePos;

                        // Highlight new tile under mouse if valid
                        if (map_->IsInFov(mousePos_)
                            && (maxRange == 0.0f
                                || player_->GetDistance(mousePos_.x,
                                                        mousePos_.y)
                                       <= maxRange))
                        {
                            TCOD_console_set_char_background(
                                console_, mousePos_.x, mousePos_.y,
                                color::white, TCOD_BKGND_SET);
                        }

                        // Present the updated frame
                        TCOD_context_present(context_, console_, nullptr);
                    }
                }

                if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                    && sdlEvent.button.button == SDL_BUTTON_LEFT)
                {
                    // Check if clicked tile is valid
                    if (map_->IsInFov(mousePos_)
                        && (maxRange == 0.0f
                            || player_->GetDistance(mousePos_.x, mousePos_.y)
                                   <= maxRange))
                    {
                        *x = mousePos_.x;
                        *y = mousePos_.y;
                        return true;
                    }
                }

                if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                    && sdlEvent.button.button == SDL_BUTTON_RIGHT)
                {
                    return false;
                }

                if (sdlEvent.type == SDL_EVENT_KEY_DOWN
                    && sdlEvent.key.key == SDLK_ESCAPE)
                {
                    return false;
                }

                if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
                {
                    SDL_Keycode key = sdlEvent.key.key;
                    pos_t delta{ 0, 0 };

                    // Map keys to movement (matching MainGameEventHandler
                    // pattern)
                    if (key == SDLK_UP)
                    {
                        delta = pos_t{ 0, -1 };
                    }
                    else if (key == SDLK_DOWN)
                    {
                        delta = pos_t{ 0, 1 };
                    }
                    else if (key == SDLK_LEFT)
                    {
                        delta = pos_t{ -1, 0 };
                    }
                    else if (key == SDLK_RIGHT)
                    {
                        delta = pos_t{ 1, 0 };
                    }
                    else if (key == SDLK_RETURN || key == SDLK_SPACE)
                    {
                        // Confirm selection with Enter or Space
                        if (map_->IsInFov(mousePos_)
                            && (maxRange == 0.0f
                                || player_->GetDistance(mousePos_.x,
                                                        mousePos_.y)
                                       <= maxRange))
                        {
                            *x = mousePos_.x;
                            *y = mousePos_.y;
                            return true;
                        }
                    }

                    // If we got a movement delta, update cursor position
                    if (delta.x != 0 || delta.y != 0)
                    {
                        pos_t newPos = mousePos_ + delta;

                        // Clamp to map bounds
                        if (map_->IsInBounds(newPos))
                        {
                            // Restore old position to range-highlighted color
                            if (lastMousePos.x >= 0 && lastMousePos.y >= 0
                                && map_->IsInFov(lastMousePos)
                                && (maxRange == 0.0f
                                    || player_->GetDistance(lastMousePos.x,
                                                            lastMousePos.y)
                                           <= maxRange))
                            {
                                tcod::ColorRGB col =
                                    originalColors[lastMousePos.x
                                                   + lastMousePos.y
                                                         * map_->GetWidth()];
                                col.r = std::min(
                                    255, static_cast<int>(col.r * 1.2f));
                                col.g = std::min(
                                    255, static_cast<int>(col.g * 1.2f));
                                col.b = std::min(
                                    255, static_cast<int>(col.b * 1.2f));
                                TCOD_console_set_char_background(
                                    console_, lastMousePos.x, lastMousePos.y,
                                    col, TCOD_BKGND_SET);
                            }

                            mousePos_ = newPos;
                            lastMousePos = newPos;

                            // Highlight new position
                            if (map_->IsInFov(mousePos_)
                                && (maxRange == 0.0f
                                    || player_->GetDistance(mousePos_.x,
                                                            mousePos_.y)
                                           <= maxRange))
                            {
                                TCOD_console_set_char_background(
                                    console_, mousePos_.x, mousePos_.y,
                                    color::white, TCOD_BKGND_SET);
                            }

                            // Present the updated frame
                            TCOD_context_present(context_, console_, nullptr);
                        }
                    }
                }
            }
        }

        return false;
    }

    Entity* Engine::GetActor(int x, int y) const
    {
        for (const auto& entity : entities_)
        {
            if (entity->GetPos().x == x && entity->GetPos().y == y
                && entity->GetDestructible()
                && !entity->GetDestructible()->IsDead())
            {
                return entity.get();
            }
        }
        return nullptr;
    }

    void Engine::DealDamage(Entity& target, unsigned int damage)
    {
        target.GetDestructible()->TakeDamage(damage);

        if (target.GetDestructible()->IsDead())
        {
            auto action = DieAction(*this, target);
            std::unique_ptr<Event> event = std::make_unique<DieAction>(action);
            AddEventFront(event);
        }
    }

    void Engine::Render()
    {
        // Clear the console - this is our drawing buffer
        TCOD_console_clear(console_);
        if (windowState_ == MainGame)
        {
            // Render the map to the console
            map_->Render(console_);
            // Show all entities' positions if in fov
            for (const auto& entity : entities_)
            {
                const auto pos = entity->GetPos();
                if (map_->IsInFov(pos))
                {
                    const auto& renderable = entity->GetRenderable();
                    renderable->Render(console_, pos);
                }
            }
            // Render UI elements on top
            healthBar_->Render(console_);
            messageLogWindow_->Render(console_);
            messageLogWindow_->RenderMouseLook(console_, *this);
        }
        else if (windowState_ == MessageHistory)
        {
            messageHistoryWindow_->Render(console_);
        }
        else if (windowState_ == Inventory)
        {
            // Render game state underneath
            map_->Render(console_);

            for (const auto& entity : entities_)
            {
                const auto pos = entity->GetPos();
                if (map_->IsInFov(pos))
                {
                    const auto& renderable = entity->GetRenderable();
                    renderable->Render(console_, pos);
                }
            }

            // Render UI elements
            healthBar_->Render(console_);
            messageLogWindow_->Render(console_);

            // Render inventory on top
            inventoryWindow_->Render(console_);
        }
        // Present the console to the screen - this actually shows what we
        // drew
        TCOD_context_present(context_, console_, nullptr);
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
                                   currentLevel_.generation.maxRoomSize, width,
                                   height });

        map_->Generate(generator);
        map_->Update();
    }

    void Engine::ProcessDeferredRemovals()
    {
        for (Entity* entity : entitiesToRemove_)
        {
            // Get entity info before removing
            std::string corpseName = "remains of " + entity->GetName();
            pos_t corpsePos = entity->GetPos();

            // Create corpse item (non-pickable, renders on CORPSES layer)
            auto corpse = std::make_unique<BaseEntity>(
                corpsePos, corpseName,
                false,                            // doesn't block movement
                AttackerComponent{ 0 },           // no attack
                DestructibleComponent{ 0, 1, 1 }, // minimal hp (not used)
                IconRenderable{ color::dark_red, '%' }, // red % symbol
                Faction::NEUTRAL,
                nullptr, // no item component
                false,   // NOT pickable
                true     // IS a corpse (renders on CORPSES layer)
            );

            // Spawn corpse at front (bottom render layer)
            SpawnEntity(std::move(corpse), corpsePos, true);

            // Now safe to remove the entity
            RemoveEntity(entity);
        }

        // Clear the removal queue
        entitiesToRemove_.clear();
    }
} // namespace tutorial
