#include "Item.hpp"

#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "StringTable.hpp"

namespace tutorial
{
    HealthPotion::HealthPotion(unsigned int amount) : amount_(amount)
    {
    }

    bool HealthPotion::Use(Entity& owner, Engine& engine)
    {
        auto* destructible = owner.GetDestructible();
        if (destructible)
        {
            unsigned int amountHealed = destructible->Heal(amount_);
            if (amountHealed > 0)
            {
                auto msg = StringTable::Instance().GetMessage(
                    "items.health_potion.use_success",
                    { { "amount", std::to_string(amountHealed) } });
                engine.LogMessage(msg.text, msg.color, msg.stack);
                return true;
            }
            else
            {
                auto msg = StringTable::Instance().GetMessage(
                    "items.health_potion.use_fail");
                engine.LogMessage(msg.text, msg.color, msg.stack);
                return false;
            }
        }
        return false;
    }

    LightningBolt::LightningBolt(float range, float damage) :
        range_(range), damage_(damage)
    {
    }

    bool LightningBolt::Use(Entity& owner, Engine& engine)
    {
        Entity* closestMonster = engine.GetClosestMonster(
            owner.GetPos().x, owner.GetPos().y, range_);

        if (!closestMonster)
        {
            auto msg = StringTable::Instance().GetMessage(
                "items.lightning_scroll.use_fail");
            engine.LogMessage(msg.text, msg.color, msg.stack);
            return false;
        }

        // Hit closest monster
        auto msg = StringTable::Instance().GetMessage(
            "items.lightning_scroll.use_success",
            { { "target", closestMonster->GetName() },
              { "damage", std::to_string(static_cast<int>(damage_)) } });
        engine.LogMessage(msg.text, msg.color, msg.stack);

        engine.DealDamage(*closestMonster, static_cast<unsigned int>(damage_));
        return true;
    }

    Fireball::Fireball(float range, float damage) :
        range_(range), damage_(damage)
    {
    }

    bool Fireball::Use(Entity& owner, Engine& engine)
    {
        auto promptMsg =
            StringTable::Instance().GetMessage("items.fireball_scroll.prompt");
        engine.LogMessage(promptMsg.text, promptMsg.color, promptMsg.stack);

        // Close inventory to allow targeting
        engine.ReturnToMainGame();

        int x, y;
        if (!engine.PickATile(&x, &y, range_))
        {
            return false;
        }

        // Burn everything in range (including player)
        auto explosionMsg = StringTable::Instance().GetMessage(
            "items.fireball_scroll.explosion",
            { { "radius", std::to_string(static_cast<int>(range_)) } });
        engine.LogMessage(explosionMsg.text, explosionMsg.color,
                          explosionMsg.stack);

        for (const auto& entity : engine.GetEntities())
        {
            if (entity->GetDestructible()
                && !entity->GetDestructible()->IsDead()
                && entity->GetDistance(x, y) <= range_)
            {
                auto hitMsg = StringTable::Instance().GetMessage(
                    "items.fireball_scroll.hit",
                    { { "target", entity->GetName() },
                      { "damage",
                        std::to_string(static_cast<int>(damage_)) } });
                engine.LogMessage(hitMsg.text, hitMsg.color, hitMsg.stack);

                engine.DealDamage(*entity, static_cast<unsigned int>(damage_));
            }
        }

        return true;
    }

    Confuser::Confuser(int nbTurns, float range) :
        nbTurns_(nbTurns), range_(range)
    {
    }

    bool Confuser::Use(Entity& owner, Engine& engine)
    {
        auto promptMsg =
            StringTable::Instance().GetMessage("items.confusion_scroll.prompt");
        engine.LogMessage(promptMsg.text, promptMsg.color, promptMsg.stack);

        // Close inventory to allow targeting
        engine.ReturnToMainGame();

        int x, y;
        if (!engine.PickATile(&x, &y, range_))
        {
            return false;
        }

        Entity* target = engine.GetActor(x, y);
        if (!target)
        {
            auto msg = StringTable::Instance().GetMessage(
                "items.confusion_scroll.use_fail");
            engine.LogMessage(msg.text, msg.color, msg.stack);
            return false;
        }

        // Only NPCs can be confused
        if (auto* npc = dynamic_cast<Npc*>(target))
        {
            auto successMsg = StringTable::Instance().GetMessage(
                "items.confusion_scroll.use_success",
                { { "target", target->GetName() } });
            engine.LogMessage(successMsg.text, successMsg.color,
                              successMsg.stack);

            // Swap in confused AI, storing the old one
            auto confusedAi = std::make_unique<ConfusedMonsterAi>(
                nbTurns_, npc->SwapAi(nullptr));
            npc->SwapAi(std::move(confusedAi));
        }
        else
        {
            auto msg = StringTable::Instance().GetMessage(
                "items.confusion_scroll.invalid_target");
            engine.LogMessage(msg.text, msg.color, msg.stack);
            return false;
        }

        return true;
    }

} // namespace tutorial
