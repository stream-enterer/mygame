#include "Event.hpp"
#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "SaveManager.hpp"
#include "StringTable.hpp"
#include "TemplateRegistry.hpp"
#include "Util.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace tutorial
{
	MessageHistoryEvent::MessageHistoryEvent(Engine& engine)
	    : EngineEvent(engine)
	{
	}

	void MessageHistoryEvent::Execute()
	{
		engine_.ShowMessageHistory();
	}
} // namespace tutorial

namespace tutorial
{
	NewGameEvent::NewGameEvent(Engine& engine) : EngineEvent(engine)
	{
	}

	void NewGameEvent::Execute()
	{
		engine_.ShowCharacterCreation();
	}
} // namespace tutorial

namespace tutorial
{
	ReturnToGameEvent::ReturnToGameEvent(Engine& engine)
	    : EngineEvent(engine)
	{
	}

	void ReturnToGameEvent::Execute()
	{
		engine_.ReturnToMainGame();
	}
} // namespace tutorial

namespace tutorial
{
	QuitEvent::QuitEvent(Engine& engine) : EngineEvent(engine)
	{
	}

	void QuitEvent::Execute()
	{
		engine_.Quit();
	}
} // namespace tutorial

namespace tutorial
{
	void Action::Execute()
	{
		if (!engine_.IsValid(entity_)) {
			return;
		}
	}
} // namespace tutorial

namespace tutorial
{
	AiAction::AiAction(Engine& engine, Entity& entity)
	    : Action(engine, entity)
	{
	}

	void AiAction::Execute()
	{
		Action::Execute();

		entity_.Act(engine_);
	}
} // namespace tutorial

namespace tutorial
{
	DieAction::DieAction(Engine& engine, Entity& entity)
	    : Action(engine, entity)
	{
	}

	void DieAction::Execute()
	{
		Action::Execute();

		// Update visual state
		entity_.Die();

		if (engine_.IsPlayer(entity_)) {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.death.player");
			engine_.LogMessage(msg.text, msg.color, msg.stack);

			// Wipe save on player death (which triggers game over)
			SaveManager::Instance().DeleteSave();
		} else {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.death.npc",
			    { { "name",
			        util::capitalize(entity_.GetName()) } });
			engine_.LogMessage(msg.text, msg.color, msg.stack);

			// Grant XP to player when monster dies
			if (entity_.GetDestructible()) {
				unsigned int xpReward =
				    entity_.GetDestructible()->GetXpReward();
				if (xpReward > 0) {
					engine_.GrantXpToPlayer(xpReward);
				}
			}
		}

		engine_.HandleDeathEvent(entity_);
	}
} // namespace tutorial

namespace tutorial
{
	WaitAction::WaitAction(Engine& engine, Entity& entity)
	    : Action(engine, entity)
	{
	}

	void WaitAction::Execute()
	{
		// No op
	}
} // namespace tutorial

namespace tutorial
{
	BumpAction::BumpAction(Engine& engine, Entity& entity, pos_t pos)
	    : DirectionalAction(engine, entity, pos)
	{
	}

	void BumpAction::Execute()
	{
		Action::Execute();

		if (entity_.GetDestructible()->IsDead()) {
			return;
		}

		auto targetPos = entity_.GetPos() + pos_;

		// Execute the resolved action directly instead of queueing it
		if (engine_.GetBlockingEntity(targetPos)) {
			MeleeAction(engine_, entity_, pos_).Execute();
		} else {
			MoveAction(engine_, entity_, pos_).Execute();
		}
	}
} // namespace tutorial

namespace tutorial
{
	MeleeAction::MeleeAction(Engine& engine, Entity& entity, pos_t pos)
	    : DirectionalAction(engine, entity, pos)
	{
	}

	void MeleeAction::Execute()
	{
		Action::Execute();

		if (!entity_.GetAttacker()) {
			return;
		}

		auto targetPos = entity_.GetPos() + pos_;
		auto* target = engine_.GetBlockingEntity(targetPos);

		if (target && !target->GetDestructible()->IsDead()) {
			auto* attacker = entity_.GetAttacker();
			auto* defender = target->GetDestructible();
			auto damage =
			    attacker->Attack() - defender->GetDefense();

			if (damage > 0) {
				auto msg = StringTable::Instance().GetMessage(
				    "messages.combat.attack_hit",
				    { { "attacker",
				        util::capitalize(entity_.GetName()) },
				      { "target", target->GetName() },
				      { "damage", std::to_string(damage) } });
				engine_.LogMessage(msg.text, msg.color,
				                   msg.stack);

				engine_.DealDamage(*target, damage);
			} else {
				auto msg = StringTable::Instance().GetMessage(
				    "messages.combat.attack_miss",
				    { { "attacker",
				        util::capitalize(entity_.GetName()) },
				      { "target", target->GetName() } });
				engine_.LogMessage(msg.text, msg.color,
				                   msg.stack);
			}
		}
	}
} // namespace tutorial

namespace tutorial
{
	MoveAction::MoveAction(Engine& engine, Entity& entity, pos_t pos)
	    : DirectionalAction(engine, entity, pos)
	{
	}

	void MoveAction::Execute()
	{
		Action::Execute();

		auto targetPos = entity_.GetPos() + pos_;

		if (engine_.IsInBounds(targetPos)
		    && !engine_.IsWall(targetPos)) {
			entity_.SetPos(entity_.GetPos() + pos_);

			// FOV computation handled by Engine::HandleEvents()
			// post-processing This separates rendering concerns
			// from movement logic
		}
	}
} // namespace tutorial

namespace tutorial
{
	PickupAction::PickupAction(Engine& engine, Entity& entity)
	    : Action(engine, entity)
	{
	}

	void PickupAction::Execute()
	{
		Action::Execute();

		pos_t entityPos = entity_.GetPos();

		// Collect all pickable items at this position
		std::vector<Entity*> itemsHere;
		const auto& entities = engine_.GetEntities();

		for (const auto& actor : entities) {
			bool isPickableItem =
			    actor->GetItem() && actor->GetPos() == entityPos
			    && !actor->IsBlocker() && actor->IsPickable();

			if (isPickableItem) {
				itemsHere.push_back(actor.get());
			}
		}

		if (itemsHere.empty()) {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.pickup.fail");
			engine_.LogMessage(msg.text, msg.color, msg.stack);
			return;
		}

		// If only one item, pick it up directly
		if (itemsHere.size() == 1) {
			PickupItemAction(engine_, entity_, itemsHere[0])
			    .Execute();
		} else {
			// Multiple items - show selection menu
			engine_.ShowItemSelection(itemsHere);
		}
	}
} // namespace tutorial

namespace tutorial
{
	PickupItemAction::PickupItemAction(Engine& engine, Entity& entity,
	                                   Entity* item)
	    : Action(engine, entity), item_(item)
	{
	}

	void PickupItemAction::Execute()
	{
		Action::Execute();

		if (!item_) {
			return;
		}

		auto* player = dynamic_cast<Player*>(&entity_);
		if (!player) {
			return;
		}

		std::string itemName = item_->GetName();
		bool success =
		    player->AddToInventory(engine_.RemoveEntity(item_));

		if (success) {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.pickup.success",
			    { { "item", itemName } });
			engine_.LogMessage(msg.text, msg.color, msg.stack);
		} else {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.pickup.inventory_full");
			engine_.LogMessage(msg.text, msg.color, msg.stack);
		}

		// Close the item selection menu
		engine_.ReturnToMainGame();
	}
} // namespace tutorial

namespace tutorial
{
	UseItemAction::UseItemAction(Engine& engine, Entity& entity,
	                             size_t itemIndex)
	    : Action(engine, entity), itemIndex_(itemIndex)
	{
	}

	void UseItemAction::Execute()
	{
		Action::Execute();

		auto* player = dynamic_cast<Player*>(&entity_);
		if (!player) {
			engine_.LogMessage("[DEBUG] Not a player",
			                   { 255, 0, 0 }, false);
			return;
		}

		if (itemIndex_ >= player->GetInventorySize()) {
			engine_.LogMessage("[DEBUG] Index out of bounds",
			                   { 255, 0, 0 }, false);
			return;
		}

		Entity* item = player->GetInventoryItem(itemIndex_);
		if (!item || !item->GetItem()) {
			engine_.LogMessage("[DEBUG] Item null or not usable",
			                   { 255, 0, 0 }, false);
			return;
		}

		int stackCountBefore = item->GetStackCount();

		engine_.LogMessage(
		    "[DEBUG] Before: stack=" + std::to_string(stackCountBefore),
		    { 255, 255, 0 }, false);

		bool wasUsed = item->GetItem()->Use(*player, engine_);

		engine_.LogMessage(
		    "[DEBUG] Use returned: "
		        + std::string(wasUsed ? "true" : "false"),
		    { 255, 255, 0 }, false);

		if (!wasUsed) {
			return;
		}

		if (itemIndex_ >= player->GetInventorySize()) {
			engine_.LogMessage(
			    "[DEBUG] Inventory changed during use",
			    { 255, 0, 0 }, false);
			return;
		}

		Entity* itemAfterUse = player->GetInventoryItem(itemIndex_);
		if (!itemAfterUse) {
			engine_.LogMessage("[DEBUG] Item became null",
			                   { 255, 0, 0 }, false);
			return;
		}

		int stackCountAfter = itemAfterUse->GetStackCount();
		engine_.LogMessage(
		    "[DEBUG] After: stack=" + std::to_string(stackCountAfter),
		    { 0, 255, 255 }, false);

		if (stackCountBefore <= 1) {
			engine_.LogMessage("[DEBUG] Removing last item",
			                   { 0, 255, 255 }, false);
			player->RemoveFromInventory(itemIndex_);
		} else {
			engine_.LogMessage(
			    "[DEBUG] Decrementing: "
			        + std::to_string(stackCountBefore) + " -> "
			        + std::to_string(stackCountBefore - 1),
			    { 0, 255, 255 }, false);
			itemAfterUse->SetStackCount(stackCountBefore - 1);

			int finalCount = itemAfterUse->GetStackCount();
			engine_.LogMessage("[DEBUG] Final count: "
			                       + std::to_string(finalCount),
			                   { 0, 255, 0 }, false);
		}
	}
} // namespace tutorial

namespace tutorial
{
	// Helper to create and spawn a single dropped item
	static void SpawnDroppedItem(Engine& engine,
	                             const std::string& templateId,
	                             pos_t dropPos, const std::string& itemName)
	{
		auto droppedItem =
		    TemplateRegistry::Instance().Create(templateId, dropPos);
		if (!droppedItem) {
			return;
		}

		int newPriority =
		    engine.GetMaxRenderPriorityAtPosition(dropPos) + 1;
		droppedItem->SetRenderPriority(newPriority);
		droppedItem->SetStackCount(1);

		engine.SpawnEntity(std::move(droppedItem), dropPos);

		auto msg = StringTable::Instance().GetMessage(
		    "messages.drop.success", { { "item", itemName } });
		engine.LogMessage(msg.text, msg.color, msg.stack);

		engine.ReturnToMainGame();
	}

	// Helper to drop entire item entity
	static void DropEntireItem(Engine& engine, Player* player,
	                           size_t itemIndex,
	                           const std::string& itemName)
	{
		auto extractedItem = player->ExtractFromInventory(itemIndex);
		if (!extractedItem) {
			return;
		}

		pos_t dropPos = player->GetPos();
		int newPriority =
		    engine.GetMaxRenderPriorityAtPosition(dropPos) + 1;
		extractedItem->SetRenderPriority(newPriority);

		engine.SpawnEntity(std::move(extractedItem), dropPos);

		auto msg = StringTable::Instance().GetMessage(
		    "messages.drop.success", { { "item", itemName } });
		engine.LogMessage(msg.text, msg.color, msg.stack);

		engine.ReturnToMainGame();
	}

	DropItemAction::DropItemAction(Engine& engine, Entity& entity,
	                               size_t itemIndex)
	    : Action(engine, entity), itemIndex_(itemIndex)
	{
	}

	void DropItemAction::Execute()
	{
		Action::Execute();

		auto* player = dynamic_cast<Player*>(&entity_);
		if (!player) {
			return;
		}

		Entity* item = player->GetInventoryItem(itemIndex_);
		if (!item) {
			return;
		}

		int stackCount = item->GetStackCount();
		std::string itemName = item->GetName();
		pos_t dropPos = player->GetPos();

		if (stackCount > 1) {
			// Drop one from stack and decrement
			SpawnDroppedItem(engine_, item->GetTemplateId(),
			                 dropPos, itemName);
			item->SetStackCount(stackCount - 1);
		} else {
			// Drop the entire item (last in stack)
			DropEntireItem(engine_, player, itemIndex_, itemName);
		}
	}
} // namespace tutorial

namespace tutorial
{
	InventoryEvent::InventoryEvent(Engine& engine) : EngineEvent(engine)
	{
	}

	void InventoryEvent::Execute()
	{
		engine_.ShowInventory();
	}
} // namespace tutorial
