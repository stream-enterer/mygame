#ifndef MESSAGE_LOG_HPP
#define MESSAGE_LOG_HPP

#include "Message.hpp"

#include <libtcod/color.hpp>

#include <string>
#include <vector>

namespace tutorial
{
    class MessageLog
    {
    public:
        void AddMessage(const std::string& text, tcod::ColorRGB color,
                        bool stack);
        void Clear();

        const std::vector<Message>& GetMessages() const
        {
            return messages_;
        }

    private:
        std::vector<Message> messages_;
    };
} // namespace tutorial

#endif // MESSAGE_LOG_HPP
