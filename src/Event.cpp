#include "Event.hpp"
#include <memory>
#include <string>
#include <vector>

#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "StringTable.hpp"
#include "Util.hpp"

namespace tutorial
{
    MessageHistoryEvent::MessageHistoryEvent(Engine& engine) :
        EngineEvent(engine)
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
        engine_.NewGame();
    }
} // namespace tutorial

namespace tutorial
{
    ReturnToGameEvent::ReturnToGameEvent(Engine& engine) : EngineEvent(engine)
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
        if (!engine_.IsValid(entity_))
        {
            return;
        }
    }
} // namespace tutorial

namespace tutorial
{
    AiAction::AiAction(Engine& engine, Entity& entity) : Action(engine, entity)
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
    DieAction::DieAction(Engine& engine, Entity& entity) :
        Action(engine, entity)
    {
    }

    void DieAction::Execute()
    {
        Action::Execute();

        if (engine_.IsPlayer(entity_))
        {
            auto msg =
                StringTable::Instance().GetMessage("messages.death.player");
            engine_.LogMessage(msg.text, msg.color, msg.stack);
        }
        else
        {
            auto msg = StringTable::Instance().GetMessage(
                "messages.death.npc",
                { { "name", util::capitalize(entity_.GetName()) } });
            engine_.LogMessage(msg.text, msg.color, msg.stack);
        }

        engine_.HandleDeathEvent(entity_);
    }
} // namespace tutorial

namespace tutorial
{
    WaitAction::WaitAction(Engine& engine, Entity& entity) :
        Action(engine, entity)
    {
    }

    void WaitAction::Execute()
    {
        // No op
    }
} // namespace tutorial

namespace tutorial
{
    BumpAction::BumpAction(Engine& engine, Entity& entity, pos_t pos) :
        DirectionalAction(engine, entity, pos)
    {
    }

    void BumpAction::Execute()
    {
        Action::Execute();

        if (entity_.GetDestructible()->IsDead())
        {
            return;
        }

        auto targetPos = entity_.GetPos() + pos_;

        if (engine_.GetBlockingEntity(targetPos))
        {
            auto action = MeleeAction(engine_, entity_, pos_);
            std::unique_ptr<Event> event =
                std::make_unique<MeleeAction>(action);
            engine_.AddEventFront(event);
        }
        else
        {
            auto action = MoveAction(engine_, entity_, pos_);
            std::unique_ptr<Event> event = std::make_unique<MoveAction>(action);
            engine_.AddEventFront(event);
        }
    }
} // namespace tutorial

namespace tutorial
{
    MeleeAction::MeleeAction(Engine& engine, Entity& entity, pos_t pos) :
        DirectionalAction(engine, entity, pos)
    {
    }

    void MeleeAction::Execute()
    {
        Action::Execute();

        if (!entity_.GetAttacker())
        {
            return;
        }

        auto targetPos = entity_.GetPos() + pos_;
        auto* target = engine_.GetBlockingEntity(targetPos);

        if (target && !target->GetDestructible()->IsDead())
        {
            auto* attacker = entity_.GetAttacker();
            auto* defender = target->GetDestructible();
            auto damage = attacker->Attack() - defender->GetDefense();

            if (damage > 0)
            {
                auto msg = StringTable::Instance().GetMessage(
                    "messages.combat.attack_hit",
                    { { "attacker", util::capitalize(entity_.GetName()) },
                      { "target", target->GetName() },
                      { "damage", std::to_string(damage) } });
                engine_.LogMessage(msg.text, msg.color, msg.stack);

                engine_.DealDamage(*target, damage);
            }
            else
            {
                auto msg = StringTable::Instance().GetMessage(
                    "messages.combat.attack_miss",
                    { { "attacker", util::capitalize(entity_.GetName()) },
                      { "target", target->GetName() } });
                engine_.LogMessage(msg.text, msg.color, msg.stack);
            }
        }
    }
} // namespace tutorial

namespace tutorial
{
    MoveAction::MoveAction(Engine& engine, Entity& entity, pos_t pos) :
        DirectionalAction(engine, entity, pos)
    {
    }

    void MoveAction::Execute()
    {
        Action::Execute();

        auto targetPos = entity_.GetPos() + pos_;

        if (engine_.IsInBounds(targetPos) && !engine_.IsWall(targetPos))
        {
            auto pos = entity_.GetPos() + pos_;

            entity_.SetPos(pos);

            if (engine_.IsPlayer(entity_))
            {
                engine_.ComputeFOV();
            }
        }
    }
} // namespace tutorial

namespace tutorial
{
    PickupAction::PickupAction(Engine& engine, Entity& entity) :
        Action(engine, entity)
    {
    }

    void PickupAction::Execute()
    {
        Action::Execute();

        auto entityPos = entity_.GetPos();

        // Collect all pickable items at this position
        std::vector<Entity*> itemsHere;
        const auto& entities = engine_.GetEntities();

        for (auto it = entities.begin(); it != entities.end(); ++it)
        {
            const auto& actor = *it;

            if (actor->GetItem() && actor->GetPos() == entityPos
                && !actor->IsBlocker() && actor->IsPickable())
            {
                itemsHere.push_back(actor.get());
            }
        }

        if (itemsHere.empty())
        {
            auto msg =
                StringTable::Instance().GetMessage("messages.pickup.fail");
            engine_.LogMessage(msg.text, msg.color, msg.stack);
            return;
        }

        // If only one item, pick it up directly
        if (itemsHere.size() == 1)
        {
            auto action = PickupItemAction(engine_, entity_, itemsHere[0]);
            std::unique_ptr<Event> event =
                std::make_unique<PickupItemAction>(action);
            engine_.AddEventFront(event);
        }
        else
        {
            // Multiple items - show selection menu
            engine_.ShowItemSelection(itemsHere);
        }
    }
} // namespace tutorial

namespace tutorial
{
    PickupItemAction::PickupItemAction(Engine& engine, Entity& entity,
                                       Entity* item) :
        Action(engine, entity), item_(item)
    {
    }

    void PickupItemAction::Execute()
    {
        Action::Execute();

        if (!item_)
        {
            return;
        }

        // Try to pick up the item (only works for Player)
        if (auto* player = dynamic_cast<Player*>(&entity_))
        {
            auto actorName = item_->GetName();

            // Remove from world and add to inventory
            if (player->AddToInventory(engine_.RemoveEntity(item_)))
            {
                auto msg = StringTable::Instance().GetMessage(
                    "messages.pickup.success", { { "item", actorName } });
                engine_.LogMessage(msg.text, msg.color, msg.stack);

                // Close the item selection menu after successful pickup
                engine_.ReturnToMainGame();
            }
            else
            {
                auto msg = StringTable::Instance().GetMessage(
                    "messages.pickup.inventory_full");
                engine_.LogMessage(msg.text, msg.color, msg.stack);

                // Close menu even if inventory is full
                engine_.ReturnToMainGame();
            }
        }
    }
} // namespace tutorial

namespace tutorial
{
    UseItemAction::UseItemAction(Engine& engine, Entity& entity,
                                 size_t itemIndex) :
        Action(engine, entity), itemIndex_(itemIndex)
    {
    }

    void UseItemAction::Execute()
    {
        Action::Execute();

        if (auto* player = dynamic_cast<Player*>(&entity_))
        {
            if (Entity* item = player->GetInventoryItem(itemIndex_))
            {
                if (item->GetItem())
                {
                    if (item->GetItem()->Use(*player, engine_))
                    {
                        // Item was used successfully, remove from inventory
                        player->RemoveFromInventory(itemIndex_);
                    }
                }
            }
        }
    }
} // namespace tutorial

namespace tutorial
{
    DropItemAction::DropItemAction(Engine& engine, Entity& entity,
                                   size_t itemIndex) :
        Action(engine, entity), itemIndex_(itemIndex)
    {
    }

    void DropItemAction::Execute()
    {
        Action::Execute();

        if (auto* player = dynamic_cast<Player*>(&entity_))
        {
            if (Entity* item = player->GetInventoryItem(itemIndex_))
            {
                std::string itemName = item->GetName();

                auto extractedItem = player->ExtractFromInventory(itemIndex_);
                if (extractedItem)
                {
                    pos_t dropPos = player->GetPos();
                    
                    // Auto-assign render priority: higher than anything at this position
                    int newPriority = engine_.GetMaxRenderPriorityAtPosition(dropPos) + 1;
                    extractedItem->SetRenderPriority(newPriority);
                    
                    // Spawn with proper priority (sorting handles placement)
                    engine_.SpawnEntity(std::move(extractedItem), dropPos);

                    auto msg = StringTable::Instance().GetMessage(
                        "messages.drop.success", { { "item", itemName } });
                    engine_.LogMessage(msg.text, msg.color, msg.stack);

                    engine_.ReturnToMainGame();
                }
            }
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
