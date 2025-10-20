#include "Item.hpp"

#include "Effect.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "TargetSelector.hpp"

#include <vector>

namespace tutorial
{
    // Implementation struct holds the actual data
    struct Item::Impl
    {
        std::unique_ptr<TargetSelector> selector;
        std::vector<std::unique_ptr<Effect>> effects;

        Impl(std::unique_ptr<TargetSelector> sel,
             std::vector<std::unique_ptr<Effect>> eff) :
            selector(std::move(sel)), effects(std::move(eff))
        {
        }
    };

    Item::Item(std::unique_ptr<TargetSelector> selector,
               std::vector<std::unique_ptr<Effect>> effects) :
        impl_(std::make_unique<Impl>(std::move(selector), std::move(effects)))
    {
    }

    Item::~Item() = default; // Defined here where Impl is complete

    bool Item::Use(Entity& owner, Engine& engine)
    {
        // Select targets
        std::vector<Entity*> targets;
        if (!impl_->selector->SelectTargets(owner, engine, targets))
        {
            return false; // Targeting failed or cancelled
        }

        // Apply all effects to all targets
        bool anySuccess = false;
        for (auto* target : targets)
        {
            for (const auto& effect : impl_->effects)
            {
                if (effect->ApplyTo(*target, engine))
                {
                    anySuccess = true;
                }
            }
        }

        // Item is consumed if at least one effect succeeded
        if (anySuccess)
        {
            engine.ReturnToMainGame();
        }

        return anySuccess;
    }

} // namespace tutorial
