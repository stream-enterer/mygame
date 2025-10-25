#ifndef UTIL_HPP
#define UTIL_HPP

#include "Position.hpp"

#include <string>

namespace tutorial
{
	namespace util
	{
		inline std::string capitalize(const std::string& string)
		{
			auto ret = string;

			auto ch = ret[0];
			ret[0] = std::toupper(ch);

			return ret;
		}

		constexpr int posToIndex(pos_t pos, int width)
		{
			return (pos.x + pos.y * width);
		}

		constexpr pos_t indexToPos(int i, int width)
		{
			int y = i / width;
			int x = i - y * width;

			return pos_t { x, y };
		};
	} // namespace util
} // namespace tutorial

#endif // UTIL_HPP
