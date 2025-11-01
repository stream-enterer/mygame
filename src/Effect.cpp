#include "Effect.hpp"

#include "AiComponent.hpp"
#include "Components.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "LocaleManager.hpp"
#include "Util.hpp"

namespace tutorial
{
	// HealthEffect implementation
	HealthEffect::HealthEffect(int amount, const std::string& messageKey)
	    : amount_(amount), messageKey_(messageKey)
	{
	}

	bool HealthEffect::ApplyTo(Entity& target, Engine& engine) const
	{
		auto* destructible = target.GetDestructible();
		if (!destructible) {
			return false;
		}

		// Don't apply health effects to corpses - they're already dead
		if (target.IsCorpse()) {
			return false;
		}

		if (amount_ > 0) {
			// Healing
			unsigned int healed = destructible->Heal(amount_);
			if (healed > 0) {
				if (!messageKey_.empty()) {
					auto msg =
					    LocaleManager::Instance()
					        .GetMessage(
					            messageKey_,
					            { { "target",
					                target.GetName() },
					              { "amount",
					                std::to_string(
					                    healed) } });
					engine.LogMessage(msg.text, msg.color,
					                  msg.stack);
				}
				return true;
			}
			return false;
		} else {
			// Damage
			unsigned int damage =
			    static_cast<unsigned int>(-amount_);

			if (!messageKey_.empty()) {
				auto msg = LocaleManager::Instance().GetMessage(
				    messageKey_,
				    { { "target",
				        util::capitalize(target.GetName()) },
				      { "damage", std::to_string(damage) } });
				engine.LogMessage(msg.text, msg.color,
				                  msg.stack);
			}

			engine.DealDamage(target, damage);
			return true;
		}
	}
	// AiChangeEffect implementation
	AiChangeEffect::AiChangeEffect(const std::string& aiType, int duration,
	                               const std::string& messageKey)
	    : aiType_(aiType), duration_(duration), messageKey_(messageKey)
	{
	}

	bool AiChangeEffect::ApplyTo(Entity& target, Engine& engine) const
	{
		// Only NPCs can have their AI changed
		auto* npc = dynamic_cast<Npc*>(&target);
		if (!npc) {
			return false;
		}

		if (aiType_ == "confused") {
			if (!messageKey_.empty()) {
				auto msg = LocaleManager::Instance().GetMessage(
				    messageKey_,
				    { { "target", target.GetName() } });
				engine.LogMessage(msg.text, msg.color,
				                  msg.stack);
			}

			// Create confused AI, storing the old one
			auto confusedAi = std::make_unique<ConfusedMonsterAi>(
			    duration_, npc->SwapAi(nullptr));
			npc->SwapAi(std::move(confusedAi));
			return true;
		}

		// Future AI types can be added here
		return false;
	}

} // namespace tutorial
