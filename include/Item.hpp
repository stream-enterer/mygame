#ifndef ITEM_HPP
#define ITEM_HPP

namespace tutorial
{
    class Entity;

    class Item
    {
    public:
        virtual ~Item() = default;

        virtual void Use(Entity& entity) = 0;
    };

    class HealthPotion final : public Item
    {
    public:
        void Use(Entity& entity) override;
    };
} // namespace tutorial

#endif // ITEM_HPP
