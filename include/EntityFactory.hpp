#ifndef ENTITY_FACTORY_HPP
#define ENTITY_FACTORY_HPP

#include "Entity.hpp"

#include <memory>

namespace tutorial
{
    class AEntityFactory
    {
    public:
        virtual ~AEntityFactory() = default;

        virtual std::unique_ptr<Entity> Create() = 0;
    };

    class OrcFactory : public AEntityFactory
    {
    public:
        std::unique_ptr<Entity> Create() override;
    };

    class TrollFactory : public AEntityFactory
    {
    public:
        std::unique_ptr<Entity> Create() override;
    };

    class PlayerFactory : public AEntityFactory
    {
    public:
        std::unique_ptr<Entity> Create() override;
    };
} // namespace tutorial

#endif // ENTITY_FACTORY_HPP
