#ifndef ITEM_HPP
#define ITEM_HPP

#include <memory>
#include <vector>

namespace tutorial
{
    class Entity;
    class Engine;
    class Effect;
    class TargetSelector;

    // Generic item that applies effects to selected targets
    class Item
    {
    public:
        Item(std::unique_ptr<TargetSelector> selector,
             std::vector<std::unique_ptr<Effect>> effects);

        ~Item(); // Need explicit destructor for pimpl with unique_ptr

        bool Use(Entity& owner, Engine& engine);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace tutorial

#endif // ITEM_HPP
