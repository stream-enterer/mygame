#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "ConfigManager.hpp"
#include "Configuration.hpp"
#include "EntityManager.hpp"
#include "InventoryMode.hpp"
#include "LevelConfig.hpp"
#include "MessageLog.hpp"
#include "Position.hpp"
#include "TemplateRegistry.hpp"

#include <SDL3/SDL.h>
#include <libtcod.h>

#include <deque>
#include <memory>
#include <string>

namespace tutorial
{
    enum Window
    {
        MainGame,
        MessageHistory,
        Inventory
    };

    class Entity;
    class Event;
    class EventHandler;
    class HealthBar;
    class Map;
    class MessageHistoryWindow;
    class MessageLogWindow;
    class InventoryWindow;

    class Engine
    {
        using Event_ptr = std::unique_ptr<Event>;

    public:
        explicit Engine(const Configuration& config);
        ~Engine();

        void AddEventFront(Event_ptr& event);
        void ComputeFOV();
        void GetInput();
        void HandleDeathEvent(Entity& entity);
        void HandleEvents();
        void LogMessage(const std::string& text, tcod::ColorRGB color,
                        bool stack);
        void DealDamage(Entity& target, unsigned int damage);
        void NewGame();
        Entity* GetClosestMonster(int x, int y, float range) const;
        bool PickATile(int* x, int* y, float maxRanger = 0.0f);
        Entity* GetActor(int x, int y) const;
        void ReturnToMainGame();
        void SetMousePos(pos_t pos);
        void ShowMessageHistory();
        void ShowInventory();
        void SetInventoryMode(InventoryMode mode)
        {
            inventoryMode_ = mode;
        }
        InventoryMode GetInventoryMode() const
        {
            return inventoryMode_;
        }
        void Quit();
        std::unique_ptr<Entity> RemoveEntity(Entity* entity);
        Entity* SpawnEntity(std::unique_ptr<Entity> entity, pos_t pos,
                            bool atFront = false);

        Entity* GetBlockingEntity(pos_t pos) const;
        Entity* GetPlayer() const;
        pos_t GetMousePos() const;
        TCOD_Context* GetContext() const;
        const Configuration& GetConfig() const;
        const EntityManager& GetEntities() const;
        EventHandler* GetEventHandler() const
        {
            return eventHandler_.get();
        }
        bool IsBlocker(pos_t pos) const;
        bool IsInBounds(pos_t pos) const;
        bool IsInFov(pos_t pos) const;
        bool IsPlayer(const Entity& entity) const;
        bool IsRunning() const;
        bool IsValid(Entity& entity) const;
        bool IsWall(pos_t pos) const;
        void Render();

    private:
        void AddEvent(Event_ptr& event);
        void GenerateMap(int width, int height);
        void HandleEnemyTurns();
        void ProcessDeferredRemovals();

        EntityManager entities_;
        std::deque<Event_ptr> eventQueue_;
        std::vector<Entity*> entitiesToRemove_;

        Configuration config_;
        LevelConfig currentLevel_;

        MessageLog messageLog_;

        std::unique_ptr<EventHandler> eventHandler_;
        std::unique_ptr<Map> map_;
        std::unique_ptr<MessageHistoryWindow> messageHistoryWindow_;
        std::unique_ptr<MessageLogWindow> messageLogWindow_;
        std::unique_ptr<InventoryWindow> inventoryWindow_;

        Entity* player_;
        std::unique_ptr<HealthBar> healthBar_;

        // New SDL3/libtcod context members
        TCOD_Context* context_;
        TCOD_Console* console_;
        SDL_Window* window_;

        Window windowState_;
        bool gameOver_;
        bool running_;

        pos_t mousePos_;
        InventoryMode inventoryMode_;
    };
} // namespace tutorial

#endif // ENGINE_HPP
