#ifndef ITEM_HPP
#define ITEM_HPP

namespace tutorial
{
    class Entity;
    class Engine;

    class Item
    {
    public:
        virtual ~Item() = default;

        virtual bool Use(Entity& owner, Engine& engine) = 0;
    };

    class HealthPotion final : public Item
    {
    public:
        explicit HealthPotion(unsigned int amount);

        bool Use(Entity& owner, Engine& engine) override;

    private:
        unsigned int amount_;
    };
} // namespace tutorial

#endif // ITEM_HPP
