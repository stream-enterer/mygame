#include "MessageLog.hpp"

namespace tutorial
{
    void MessageLog::AddMessage(const std::string& text, tcod::ColorRGB color,
                                bool stack)
    {
        const auto end = messages_.size() - 1;

        if (stack && messages_.size() > 0 && text == messages_[end].text)
        {
            messages_[end].count += 1;
        }
        else
        {
            messages_.emplace_back(text, color);
        }
    }

    void MessageLog::Clear()
    {
        messages_.clear();
    }
} // namespace tutorial
