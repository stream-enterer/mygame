#include "Engine.hpp"

#include "Colors.hpp"
#include "Entity.hpp"
#include "EntityFactory.hpp"
#include "Event.hpp"
#include "EventHandler.hpp"
#include "HealthBar.hpp"
#include "InventoryWindow.hpp"
#include "Map.hpp"
#include "MapGenerator.hpp"
#include "MessageHistoryWindow.hpp"
#include "MessageLogWindow.hpp"

#include <memory>
#include <stdexcept>
#include <string>

namespace tutorial
{
    inline namespace
    {
        constexpr int kUiHeight = 5;

        constexpr int kRoomMinSize = 6;
        constexpr int kRoomMaxSize = 10;
        constexpr int kMaxRooms = 30;

        constexpr int kFovRadius = 10;

        constexpr int kMaxMonstersPerRoom = 3;
    } // namespace

    // Public methods
    Engine::Engine(const Configuration& config) :
        config_(config),
        eventHandler_(std::make_unique<MainGameEventHandler>(*this)),
        map_(std::make_unique<Map>(config.width, config.height - kUiHeight)),
        messageHistoryWindow_(std::make_unique<MessageHistoryWindow>(
            config.width, config.height, pos_t{ 0, 0 }, messageLog_)),
        messageLogWindow_(std::make_unique<MessageLogWindow>(
            40, 5, pos_t{ 21, 45 }, messageLog_)),
        player_(nullptr),
        healthBar_(nullptr),
        context_(nullptr),
        console_(nullptr),
        window_(nullptr),
        windowState_(MainGame),
        gameOver_(false),
        running_(true),
        mousePos_{ 0, 0 }
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
        map_->ComputeFov(player_->GetPos(), kFovRadius);
        map_->Update();
    }

    void Engine::GetInput()
    {
        auto action = eventHandler_->Dispatch();

        if (action.get() != nullptr)
        {
            this->AddEvent(action);

            if (!gameOver_)
            {
                this->HandleEnemyTurns();
            }
        }
    }

    void Engine::HandleDeathEvent(Entity& entity)
    {
        if (this->IsPlayer(entity))
        {
            eventHandler_ = std::make_unique<GameOverEventHandler>(*this);
            eventQueue_.clear();
            gameOver_ = true;
        }

        entity.Die();

        // Move entity to front of entity list so that the corpse is
        // rendered first This prevents live entities from being rendered
        // underneath a corpse
        entities_.MoveToFront(entity);
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
    }

    void Engine::LogMessage(const std::string& text, tcod::ColorRGB color,
                            bool stack)
    {
        messageLog_.AddMessage(text, color, stack);
    }

    void Engine::NewGame()
    {
        // Clear the entities and message log
        entities_.Clear();
        messageLog_.Clear();
        eventQueue_.clear();

        this->GenerateMap(config_.width, config_.height - kUiHeight);

        // Place monsters
        auto rooms = map_->GetRooms();

        for (auto it = rooms.begin() + 1; it != rooms.end(); ++it)
        {
            entities_.PlaceEntities(*it, kMaxMonstersPerRoom);
        }

        // Place items
        for (auto it = rooms.begin() + 1; it != rooms.end(); ++it)
        {
            entities_.PlaceItems(*it, 2); // Max 2 items per room
        }

        // Create player and add them to entity list
        PlayerFactory factory{};
        player_ = entities_.Spawn(factory.Create(), rooms[0].GetCenter()).get();

        // Create health bar
        healthBar_ =
            std::make_unique<HealthBar>(20, 1, pos_t{ 0, 45 }, *player_);

        // Create inventory window
        inventoryWindow_ = std::make_unique<InventoryWindow>(
            50, 28, pos_t{ config_.width / 2 - 25, config_.height / 2 - 14 },
            *player_);

        this->ComputeFOV();

        messageLog_.AddMessage("Hello and welcome to the C++ libtcod dungeon!",
                               color::light_azure, false);

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

        while (running_)
        {
            // Render the current game state
            this->Render();

            // Highlight valid tiles in range
            for (int cx = 0; cx < map_->GetWidth(); cx++)
            {
                for (int cy = 0; cy < map_->GetHeight(); cy++)
                {
                    if (map_->IsInFov(pos_t{ cx, cy })
                        && (maxRange == 0.0f
                            || player_->GetDistance(cx, cy) <= maxRange))
                    {
                        // Get current background color and brighten it
                        TCOD_color_t tcodCol =
                            TCOD_console_get_char_background(console_, cx, cy);

                        // Convert to ColorRGB and brighten
                        tcod::ColorRGB col{ tcodCol.r, tcodCol.g, tcodCol.b };
                        col.r = std::min(255, static_cast<int>(col.r * 1.2f));
                        col.g = std::min(255, static_cast<int>(col.g * 1.2f));
                        col.b = std::min(255, static_cast<int>(col.b * 1.2f));

                        TCOD_console_set_char_background(console_, cx, cy, col,
                                                         TCOD_BKGND_SET);
                    }
                }
            }

            // Handle mouse/keyboard events
            while (SDL_PollEvent(&sdlEvent))
            {
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

                    mousePos_ = pos_t{ tileX, tileY };

                    // Highlight tile under mouse if valid
                    if (map_->IsInFov(mousePos_)
                        && (maxRange == 0.0f
                            || player_->GetDistance(mousePos_.x, mousePos_.y)
                                   <= maxRange))
                    {
                        TCOD_console_set_char_background(
                            console_, mousePos_.x, mousePos_.y, color::white,
                            TCOD_BKGND_SET);
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

                if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
                {
                    return false;
                }
            }

            TCOD_context_present(context_, console_, nullptr);
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
        Map::Generator generator(
            { kMaxRooms, kRoomMinSize, kRoomMaxSize, width, height });

        map_->Generate(generator);
        map_->Update();
    }

    void Engine::HandleEnemyTurns()
    {
        for (auto& entity : entities_)
        {
            if (IsPlayer(*entity.get()))
            {
                continue;
            }

            if (!entity->CanAct())
            {
                continue;
            }

            std::unique_ptr<Event> event =
                std::make_unique<AiAction>(*this, *entity);
            this->AddEvent(event);
        }
    }
} // namespace tutorial
