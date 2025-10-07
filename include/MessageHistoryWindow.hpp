#ifndef MESSAGE_HISTORY_WINDOW_HPP
#define MESSAGE_HISTORY_WINDOW_HPP

#include "MessageLog.hpp"
#include "Position.hpp"
#include "UiWindow.hpp"

#include <libtcod/console.hpp>

#include <cstddef>

namespace tutorial
{
    class MessageHistoryWindow : public UiWindowBase
    {
    public:
        MessageHistoryWindow(std::size_t width, std::size_t height, pos_t pos,
                             const MessageLog& log);

        virtual void Render(TCOD_Console* parent) const override;

    private:
        const MessageLog& log_;
    };
} // namespace tutorial

#endif // MESSAGE_HISTORY_WINDOW_HPP
