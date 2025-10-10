#ifndef MESSAGE_LOG_WINDOW_HPP
#define MESSAGE_LOG_WINDOW_HPP

#include "Position.hpp"
#include "UiWindow.hpp"

#include <cstddef>

#include <libtcod/console.hpp>

namespace tutorial
{
    class MessageLog;

    class Engine;

    class MessageLogWindow : public UiWindowBase
    {
    public:
        MessageLogWindow(std::size_t width, std::size_t height, pos_t pos,
                         const MessageLog& log);

        void Render(TCOD_Console* parent) const;
        void RenderMouseLook(TCOD_Console* parent, const Engine& engine) const;

    private:
        const MessageLog& log_;
    };
} // namespace tutorial

#endif // MESSAGE_LOG_WINDOW_HPP
