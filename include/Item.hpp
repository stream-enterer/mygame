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

    class LightningBolt final : public Item
    {
    public:
        LightningBolt(float range, float damage);

        bool Use(Entity& owner, Engine& engine) override;

    protected:
        float range_;
        float damage_;
    };

    class Fireball final : public Item
    {
    public:
        Fireball(float range, float damage);

        bool Use(Entity& owner, Engine& engine) override;

    private:
        float range_;
        float damage_;
    };

    class Confuser final : public Item
    {
    public:
        Confuser(int nbTurns, float range);

        bool Use(Entity& owner, Engine& engine) override;

    private:
        int nbTurns_;
        float range_;
    };
} // namespace tutorial

#endif // ITEM_HPP
