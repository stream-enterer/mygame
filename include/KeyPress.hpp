#ifndef KEY_PRESS_HPP
#define KEY_PRESS_HPP

#include <libtcod/console.hpp>

namespace tutorial
{
	struct KeyPress {
		KeyPress(TCOD_keycode_t key) : key(key), c(0)
		{
		}

		KeyPress(TCOD_keycode_t key, char c) : key(key), c(c)
		{
		}

		TCOD_keycode_t key { TCODK_NONE };
		char c { 0 };
	};

	constexpr bool operator==(const KeyPress& lhs, const KeyPress& rhs)
	{
		return (lhs.key == rhs.key && lhs.c == rhs.c);
	}

	struct KeyPressHash {
		// Would like to make constexpr, but only supported by clang
		std::size_t operator()(const KeyPress& key) const
		{
			return std::hash<TCOD_keycode_t>()(key.key)
			       ^ (std::hash<char>()(key.c) << 1);
		}
	};
} // namespace tutorial

#endif // KEY_PRESS_HPP
