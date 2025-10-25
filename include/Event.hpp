#ifndef EVENT_HPP
#define EVENT_HPP

#include "Colors.hpp"
#include "Position.hpp"

namespace tutorial
{
	class Engine;
	class Entity;

	// Events
	class Event
	{
	public:
		virtual ~Event() = default;

		virtual void Execute() = 0;
	};

	class EngineEvent : public Event
	{
	public:
		EngineEvent(Engine& engine) : engine_(engine)
		{
		}

		virtual void Execute() = 0;

	protected:
		Engine& engine_;
	};

	class MessageHistoryEvent final : public EngineEvent
	{
	public:
		MessageHistoryEvent(Engine& engine);

		void Execute() override;
	};

	class InventoryEvent final : public EngineEvent
	{
	public:
		InventoryEvent(Engine& engine);

		void Execute() override;
	};

	class NewGameEvent final : public EngineEvent
	{
	public:
		NewGameEvent(Engine& engine);

		void Execute() override;
	};

	class ReturnToGameEvent final : public EngineEvent
	{
	public:
		ReturnToGameEvent(Engine& engine);

		void Execute() override;
	};

	class QuitEvent final : public EngineEvent
	{
	public:
		QuitEvent(Engine& engine);

		void Execute() override;
	};

	// Actions
	class Action : public Event
	{
	public:
		Action(Engine& engine, Entity& entity)
		    : engine_(engine), entity_(entity)
		{
		}

		virtual void Execute();

	protected:
		Engine& engine_;
		Entity& entity_;
	};

	class AiAction final : public Action
	{
	public:
		AiAction(Engine& engine, Entity& entity);

		void Execute() override;
	};

	class DieAction final : public Action
	{
	public:
		DieAction(Engine& engine, Entity& entity);

		void Execute() override;
	};

	class WaitAction final : public Action
	{
	public:
		WaitAction(Engine& engine, Entity& entity);

		void Execute() override;
	};

	class DirectionalAction : public Action
	{
	public:
		DirectionalAction(Engine& engine, Entity& entity, pos_t pos)
		    : Action(engine, entity), pos_(pos)
		{
		}

		virtual void Execute() = 0;

	protected:
		pos_t pos_;
	};

	class BumpAction final : public DirectionalAction
	{
	public:
		BumpAction(Engine& engine, Entity& entity, pos_t pos);

		void Execute() override;
	};

	class MeleeAction final : public DirectionalAction
	{
	public:
		MeleeAction(Engine& engine, Entity& entity, pos_t pos);

		void Execute() override;
	};

	class MoveAction final : public DirectionalAction
	{
	public:
		MoveAction(Engine& engine, Entity& entity, pos_t pos);

		void Execute() override;
	};

	class PickupAction final : public Action
	{
	public:
		PickupAction(Engine& engine, Entity& entity);

		void Execute() override;
	};

	class PickupItemAction final : public Action
	{
	public:
		PickupItemAction(Engine& engine, Entity& entity, Entity* item);

		void Execute() override;

	private:
		Entity* item_;
	};

	class UseItemAction final : public Action
	{
	public:
		UseItemAction(Engine& engine, Entity& entity, size_t itemIndex);

		void Execute() override;

	private:
		size_t itemIndex_;
	};

	class DropItemAction final : public Action
	{
	public:
		DropItemAction(Engine& engine, Entity& entity,
		               size_t itemIndex);

		void Execute() override;

	private:
		size_t itemIndex_;
	};
} // namespace tutorial

#endif // EVENT_HPP
