#include "InventoryWindow.hpp"

#include "Colors.hpp"
#include "Engine.hpp"
#include "Entity.hpp"

namespace tutorial
{
    InventoryWindow::InventoryWindow(std::size_t width, std::size_t height,
                                     pos_t pos, const Entity& player) :
        UiWindowBase(width, height, pos), player_(player), title_("Inventory")
    {
    }

    void InventoryWindow::Render(TCOD_Console* parent) const
    {
        TCOD_console_clear(console_);

        // Draw frame
        TCOD_console_set_default_foreground(console_,
                                            tcod::ColorRGB{ 200, 180, 50 });
        TCOD_console_print_frame(console_, 0, 0,
                                 TCOD_console_get_width(console_),
                                 TCOD_console_get_height(console_), true,
                                 TCOD_BKGND_DEFAULT, title_.c_str());

        // Display items with shortcuts
        if (auto* player = dynamic_cast<const Player*>(&player_))
        {
            TCOD_console_set_default_foreground(
                console_, tcod::ColorRGB{ 255, 255, 255 });

            const auto& inventory = player->GetInventory();
            char shortcut = 'a';
            int y = 1;

            for (const auto& item : inventory)
            {
                TCOD_console_print(console_, 2, y, "(%c) %s", shortcut,
                                   item->GetName().c_str());
                y++;
                shortcut++;
            }

            if (inventory.empty())
            {
                TCOD_console_print(console_, 2, 1, "(empty)");
            }
        }

        // Blit to parent
        TCOD_console_blit(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), parent, pos_.x,
                          pos_.y, 1.0f, 1.0f);
    }
} // namespace tutorial
