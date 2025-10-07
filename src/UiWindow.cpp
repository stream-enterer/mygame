#include "UiWindow.hpp"

#include <libtcod/console.hpp>
#include <memory>

namespace tutorial
{
    UiWindowBase::UiWindowBase(std::size_t width, std::size_t height, pos_t pos)
        : console_(TCOD_console_new(width, height)), pos_(pos)
    {
    }

    UiWindowBase::~UiWindowBase()
    {
        if (console_)
        {
            TCOD_console_delete(console_);
        }
    }

    void UiWindowBase::Render(TCOD_Console*) const
    {
        // No op
    }
} // namespace tutorial
