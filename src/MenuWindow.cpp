#include "MenuWindow.hpp"

#include "ConfigManager.hpp"

#include <algorithm>

namespace tutorial
{
    MenuWindow::MenuWindow(std::size_t width, std::size_t height, pos_t pos,
                           const std::string& title) :
        UiWindowBase(width, height, pos), title_(title), selectedIndex_(0)
    {
    }

    void MenuWindow::Render(TCOD_Console* parent) const
    {
        if (!console_ || items_.empty())
        {
            return;
        }

        auto& cfg = ConfigManager::Instance();
        auto frameColor = cfg.GetUIFrameColor();
        auto textColor = cfg.GetUITextColor();

        // Clear console
        TCOD_console_clear(console_);

        // Draw frame
        const int width = TCOD_console_get_width(console_);
        const int height = TCOD_console_get_height(console_);

        for (int x = 0; x < width; ++x)
        {
            TCOD_console_put_char(console_, x, 0, ' ', TCOD_BKGND_NONE);
            TCOD_console_set_char_background(console_, x, 0, frameColor,
                                             TCOD_BKGND_SET);

            TCOD_console_put_char(console_, x, height - 1, ' ',
                                  TCOD_BKGND_NONE);
            TCOD_console_set_char_background(console_, x, height - 1,
                                             frameColor, TCOD_BKGND_SET);
        }

        for (int y = 1; y < height - 1; ++y)
        {
            TCOD_console_put_char(console_, 0, y, ' ', TCOD_BKGND_NONE);
            TCOD_console_set_char_background(console_, 0, y, frameColor,
                                             TCOD_BKGND_SET);

            TCOD_console_put_char(console_, width - 1, y, ' ', TCOD_BKGND_NONE);
            TCOD_console_set_char_background(console_, width - 1, y, frameColor,
                                             TCOD_BKGND_SET);
        }

        // Draw title
        TCOD_console_set_default_foreground(console_, textColor);
        int titleX = (width - static_cast<int>(title_.length())) / 2;
        TCOD_console_print(console_, titleX, 2, title_.c_str());

        // Draw menu items
        int startY = 5;
        for (size_t i = 0; i < items_.size(); ++i)
        {
            if (static_cast<int>(i) == selectedIndex_)
            {
                // Highlight selected item
                TCOD_console_set_default_foreground(
                    console_, tcod::ColorRGB{ 255, 200, 100 }); // Light orange
            }
            else
            {
                TCOD_console_set_default_foreground(console_, textColor);
            }

            int itemY = startY + static_cast<int>(i) * 2;
            int itemX =
                (width - static_cast<int>(items_[i].label.length())) / 2;
            TCOD_console_print(console_, itemX, itemY, items_[i].label.c_str());
        }

        // Blit to parent
        TCOD_console_blit(console_, 0, 0, width, height, parent, pos_.x, pos_.y,
                          1.0f, 0.8f); // Semi-transparent background
    }

    void MenuWindow::Clear()
    {
        items_.clear();
        selectedIndex_ = 0;
    }

    void MenuWindow::AddItem(MenuAction action, const std::string& label)
    {
        items_.push_back({ action, label });
    }

    void MenuWindow::SelectPrevious()
    {
        if (items_.empty())
        {
            return;
        }

        selectedIndex_--;
        if (selectedIndex_ < 0)
        {
            selectedIndex_ = static_cast<int>(items_.size()) - 1;
        }
    }

    void MenuWindow::SelectNext()
    {
        if (items_.empty())
        {
            return;
        }

        selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(items_.size());
    }

    MenuAction MenuWindow::GetSelectedAction() const
    {
        if (selectedIndex_ >= 0
            && selectedIndex_ < static_cast<int>(items_.size()))
        {
            return items_[selectedIndex_].action;
        }
        return MenuAction::None;
    }

} // namespace tutorial
