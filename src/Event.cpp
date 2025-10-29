#include "Event.hpp"
#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "SaveManager.hpp"
#include "SpellRegistry.hpp"
#include "SpellcasterComponent.hpp"
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
			return;
		}

		if (itemIndex_ >= player->GetInventorySize()) {
			return;
		}

		Entity* item = player->GetInventoryItem(itemIndex_);
		if (!item || !item->GetItem()) {
			return;
		}

		// Store stack count BEFORE using item
		int stackCountBefore = item->GetStackCount();

		// Attempt to use the item
		bool wasUsed = item->GetItem()->Use(*player, engine_);

		if (!wasUsed) {
			return;
		}

		// Item was successfully used - re-validate item still exists
		if (itemIndex_ >= player->GetInventorySize()) {
			return;
		}

		Entity* itemAfter = player->GetInventoryItem(itemIndex_);
		if (!itemAfter) {
			return;
		}

		// Consume ONE item from the stack
		if (stackCountBefore <= 1) {
			player->RemoveFromInventory(itemIndex_);
		} else {
			itemAfter->SetStackCount(stackCountBefore - 1);
		}
	}
} // namespace tutorial

namespace tutorial
{
	CastSpellAction::CastSpellAction(Engine& engine, Entity& entity,
	                                 const std::string& spellId)
	    : Action(engine, entity), spellId_(spellId)
	{
	}

	void CastSpellAction::Execute()
	{
		Action::Execute();

		// Check if entity can cast spells
		auto* caster = entity_.GetSpellcaster();
		if (!caster) {
			return;
		}

		// Check if spell is known
		if (!caster->KnowsSpell(spellId_)) {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.spell.unknown");
			engine_.LogMessage(msg.text, msg.color, msg.stack);
			return;
		}

		// Get spell data
		const SpellData* spell =
		    SpellRegistry::Instance().Get(spellId_);
		if (!spell) {
			engine_.LogMessage(
			    "[DEBUG] Spell not found: " + spellId_,
			    { 255, 0, 0 }, false);
			return;
		}

		// Check if entity has enough MP
		auto* destructible = entity_.GetDestructible();
		if (!destructible) {
			return;
		}

		if (destructible->GetMp() < spell->manaCost) {
			auto msg = StringTable::Instance().GetMessage(
			    "messages.spell.not_enough_mana");
			engine_.LogMessage(msg.text, msg.color, msg.stack);
			engine_.ReturnToMainGame();
			return;
		}

		// Create target selector and effects from spell data
		auto selector = spell->CreateTargetSelector();
		auto effects = spell->CreateEffects();

		// Select targets
		std::vector<Entity*> targets;
		if (!selector->SelectTargets(entity_, engine_, targets)) {
			return; // Targeting cancelled
		}

		// Apply effects to all targets
		bool anySuccess = false;
		for (auto* target : targets) {
			for (const auto& effect : effects) {
				if (effect->ApplyTo(*target, engine_)) {
					anySuccess = true;
				}
			}
		}

		// Spell is consumed if at least one effect succeeded
		if (anySuccess) {
			destructible->SpendMp(spell->manaCost);
			engine_.ReturnToMainGame();
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
