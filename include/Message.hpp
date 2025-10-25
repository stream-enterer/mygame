#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <libtcod/color.hpp>

#include <string>

namespace tutorial
{
	struct Message {
		Message(const std::string& text, tcod::ColorRGB color)
		    : text(text), count(1), color(color)
		{
		}

		std::string text;
		unsigned int count;
		tcod::ColorRGB color;
	};
} // namespace tutorial

#endif // MESSAGE_HPP
