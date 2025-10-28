#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Components.hpp"
#include "ConfigManager.hpp"
#include "Configuration.hpp"
#include "EntityManager.hpp"
#include "InventoryMode.hpp"
#include "LevelConfig.hpp"
#include "MessageLog.hpp"
#include "Position.hpp"
#include "TargetingCursor.hpp"
#include "TemplateRegistry.hpp"

#include <SDL3/SDL.h>
#include <libtcod.h>

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tutorial
{
	class Command;
	class Entity;
	class Event;
	class EventHandler;
	class HealthBar;
	class Map;
	class MessageHistoryWindow;
	class MessageLogWindow;
	class Menu;

	enum class GameState { MainGame, MessageHistory, InMenu };

	class Engine
	{
		using Event_ptr = std::unique_ptr<Event>;

	public:
		explicit Engine(const Configuration& config);
		~Engine();

		void AddEventFront(Event_ptr& event);
		void ComputeFOV();
		std::unique_ptr<Command> GetInput();
		void HandleDeathEvent(Entity& entity);
		void HandleEvents();
		void LogMessage(const std::string& text, tcod::ColorRGB color,
		                bool stack);
		void DealDamage(Entity& target, unsigned int damage);
		void GrantXpToPlayer(unsigned int xpAmount);
		void NewGame();
		Entity* GetClosestMonster(pos_t pos, float range) const;
		bool PickATile(
		    pos_t* pos, float maxRange = 0.0f,
		    std::function<bool(pos_t)> validator = nullptr,
		    TargetingType targetingType = TargetingType::None,
		    float radius = 0.0f);
		Entity* GetActor(pos_t pos) const;
		void SetMousePos(pos_t pos);

		// Menu stack management
		void PushMenu(std::unique_ptr<Menu> menu);
		void PopMenu();
		bool HasMenus() const
		{
			return !menuStack_.empty();
		}
		GameState GetGameState() const;

		// Menu builders
		void ShowInventory();
		void ShowItemSelection(const std::vector<Entity*>& items);
		void ShowPauseMenu();
		void ShowStartMenu();
		void ShowClassSelection();
		void ShowLevelUpMenu();
		void ReturnToMainGame();
		void ShowMessageHistory();

		void SetInventoryMode(InventoryMode mode)
		{
			inventoryMode_ = mode;
		}
		InventoryMode GetInventoryMode() const
		{
			return inventoryMode_;
		}
		const std::vector<Entity*>& GetItemSelectionList() const
		{
			return itemSelectionList_;
		}
		void HandleMenuInput(SDL_Keycode key, char character);
		void Quit();
		std::unique_ptr<Entity> RemoveEntity(Entity* entity);
		Entity* SpawnEntity(std::unique_ptr<Entity> entity, pos_t pos);

		Entity* GetBlockingEntity(pos_t pos) const;
		Entity* GetPlayer() const;
		pos_t GetMousePos() const;
		TCOD_Context* GetContext() const;
		const Configuration& GetConfig() const;
		const TCOD_ViewportOptions& GetViewportOptions() const;
		const EntityManager& GetEntities() const;
		EventHandler* GetEventHandler() const
		{
			return eventHandler_.get();
		}
		std::string GetCurrentLevelId() const;
		const Map& GetMap() const
		{
			return *map_;
		}
		int GetMaxRenderPriorityAtPosition(pos_t pos) const;
		bool IsBlocker(pos_t pos) const;
		bool IsInBounds(pos_t pos) const;
		bool IsInFov(pos_t pos) const;
		bool IsPlayer(const Entity& entity) const;
		bool IsRunning() const;
		bool IsValid(Entity& entity) const;
		bool IsGameOver() const
		{
			return gameOver_;
		}
		bool IsWall(pos_t pos) const;
		void Render();

		Entity* GetStairs() const;
		int GetDungeonLevel() const;
		void NextLevel();

		void RenderGameUI(TCOD_Console* targetConsole) const;

	private:
		friend class TurnManager;
		friend class MoveCommand;
		friend class WaitCommand;
		friend class PickupCommand;
		friend class PickupItemCommand;
		friend class DescendStairsCommand;
		friend class UseItemCommand;
		friend class DropItemCommand;
		friend class SaveManager;

		void AddEvent(Event_ptr& event);
		void GenerateMap(int width, int height);
		void ProcessDeferredRemovals();
		void EnsureInitialized();

		// Rendering helpers
		void RenderGameBackground(TCOD_Console* console);

		// Level transition helpers
		struct PlayerState {
			std::string name;
			AttackerComponent attacker;
			DestructibleComponent destructible;
			std::vector<std::unique_ptr<Entity>> inventory;

			PlayerState(const std::string& n,
			            const AttackerComponent& atk,
			            const DestructibleComponent& dest)
			    : name(n), attacker(atk), destructible(dest)
			{
			}

			PlayerState(PlayerState&&) = default;
			PlayerState& operator=(PlayerState&&) = default;
			PlayerState(const PlayerState&) = delete;
			PlayerState& operator=(const PlayerState&) = delete;
		};

		PlayerState SavePlayerState();
		void LoadLevelConfiguration(int dungeonLevel);
		void ClearCurrentLevel();
		void PopulateLevelWithEntities();
		void RestorePlayerWithState(PlayerState&& state,
		                            pos_t position);
		void RecreatePlayerUI();
		pos_t CalculateWindowPosition(int width, int height,
		                              bool center) const;

		static constexpr int kAutosaveInterval = 100;

		// Member variables in initialization order
		Configuration config_;
		LevelConfig currentLevel_;

		EntityManager entities_;
		std::deque<Event_ptr> eventQueue_;
		std::vector<Entity*> entitiesToRemove_;

		MessageLog messageLog_;

		std::unique_ptr<EventHandler> eventHandler_;
		std::unique_ptr<Map> map_;
		std::unique_ptr<MessageHistoryWindow> messageHistoryWindow_;
		std::unique_ptr<MessageLogWindow> messageLogWindow_;

		Entity* player_;
		std::unique_ptr<HealthBar> healthBar_;

		Entity* stairs_;
		int dungeonLevel_;
		int turnsSinceLastAutosave_;

		// SDL3/libtcod context members
		TCOD_Context* context_;
		tcod::TilesetPtr tileset_;
		TCOD_Console* console_;
		SDL_Window* window_;
		TCOD_ViewportOptions viewportOptions_;

		// Menu system
		std::vector<std::unique_ptr<Menu>> menuStack_;

		GameState gameState_;
		bool gameOver_;
		bool running_;

		pos_t mousePos_;
		InventoryMode inventoryMode_;
		std::vector<Entity*> itemSelectionList_;
	};
} // namespace tutorial

#endif // ENGINE_HPP
