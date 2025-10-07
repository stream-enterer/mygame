#include "MessageLogWindow.hpp"

#include "MessageLog.hpp"
#include "UiWindow.hpp"

namespace tutorial
{
    MessageLogWindow::MessageLogWindow(std::size_t width, std::size_t height,
                                       pos_t pos, const MessageLog& log)
        : UiWindowBase(width, height, pos), log_(log)
    {
    }

    void MessageLogWindow::Render(TCOD_Console* parent) const
    {
        auto messages = log_.GetMessages();

        TCOD_console_clear(console_);

        int y_offset = TCOD_console_get_height(console_) - 1;

        for (auto it = messages.rbegin(); it != messages.rend(); ++it)
        {
            auto line_height = TCOD_console_get_height_rect_fmt(
                console_, 0, 0, TCOD_console_get_width(console_),
                TCOD_console_get_height(console_), "%s", it->text.c_str());

            if (line_height > 1)
            {
                y_offset -= line_height - 1;
            }

            {
                auto* line = TCOD_console_new(TCOD_console_get_width(console_),
                                              line_height);

                TCOD_console_printf_rect(
                    line, 0, 0, TCOD_console_get_width(line),
                    TCOD_console_get_height(line), "%s", it->text.c_str());

                TCOD_console_blit(line, 0, 0, TCOD_console_get_width(line),
                                  TCOD_console_get_height(line), console_, 0,
                                  y_offset, 1.0f, 1.0f);

                TCOD_console_delete(line);
            }

            --y_offset;

            if (y_offset < 0)
            {
                break;
            }
        }

        TCOD_console_blit(console_, 0, 0, TCOD_console_get_width(console_),
                          TCOD_console_get_height(console_), parent, pos_.x,
                          pos_.y, 1.0f, 1.0f);
    }
} // namespace tutorial
