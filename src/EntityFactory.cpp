#include "EntityFactory.hpp"

#include "AiComponent.hpp"
#include "Colors.hpp"
#include "Components.hpp"
#include "Position.hpp"

namespace tutorial
{
    std::unique_ptr<Entity> OrcFactory::Create()
    {
        return std::make_unique<Npc>(
            pos_t{ 0, 0 }, "orc", true, AttackerComponent{ 3 },
            DestructibleComponent{ 0, 10, 10 },
            IconRenderable{ color::desaturated_green, 'o' },
            std::make_unique<HostileAi>());
    }

    std::unique_ptr<Entity> TrollFactory::Create()
    {
        return std::make_unique<Npc>(pos_t{ 0, 0 }, "troll", true,
                                     AttackerComponent{ 4 },
                                     DestructibleComponent{ 1, 16, 16 },
                                     IconRenderable{ color::darker_green, 'T' },
                                     std::make_unique<HostileAi>());
    }

    std::unique_ptr<Entity> PlayerFactory::Create()
    {
        return std::make_unique<Player>(pos_t{ 0, 0 }, "player", true,
                                        AttackerComponent{ 5 },
                                        DestructibleComponent{ 2, 30, 30 },
                                        IconRenderable{ color::white, '@' }

        );
    }

    std::unique_ptr<Entity> HealthPotionFactory::Create()
    {
        return std::make_unique<BaseEntity>(
            pos_t{ 0, 0 }, "health potion", false, AttackerComponent{ 0 },
            DestructibleComponent{ 0, 1, 1 },
            IconRenderable{ color::light_azure, '!' },
            std::make_unique<HealthPotion>(4));
    }
} // namespace tutorial
