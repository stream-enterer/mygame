#include "ItemSelectionWindow.hpp"

#include "Colors.hpp"
#include "ConfigManager.hpp"
#include "Entity.hpp"

namespace tutorial
{
    ItemSelectionWindow::ItemSelectionWindow(std::size_t width,
                                             std::size_t height, pos_t pos,
                                             const std::vector<Entity*>& items,
                                             const std::string& title) :
        UiWindowBase(width, height, pos), items_(items), title_(title)
    {
    }

    void ItemSelectionWindow::Render(TCOD_Console* parent) const
    {
        TCOD_console_clear(console_);

        // Draw frame
        TCOD_console_set_default_foreground(console_,
                                            ConfigManager::Instance().GetUIFrameColor());
        TCOD_console_print_frame(console_, 0, 0,
                                 TCOD_console_get_width(console_),
                                 TCOD_console_get_height(console_), true,
                                 TCOD_BKGND_DEFAULT, title_.c_str());

        // Display items with shortcuts
        TCOD_console_set_default_foreground(
            console_, ConfigManager::Instance().GetUITextColor());

        char shortcut = 'a';
        int y = 1;

        for (const auto* item : items_)
        {
            TCOD_console_print(console_, 2, y, "(%c) %s", shortcut,
                               item->GetName().c_str());
            y++;
            shortcut++;
        }

        if (items_.empty())
        {
            TCOD_console_print(console_, 2, 1, "(nothing here)");
        }

        // Blit to parent
        TCOD_console_blit(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), parent, pos_.x,
                          pos_.y, 1.0f, 1.0f);
    }
} // namespace tutorial
