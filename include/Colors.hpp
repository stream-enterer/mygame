#ifndef COLORS_HPP
#define COLORS_HPP

#include <libtcod/color.hpp>

namespace tutorial::color
{
    constexpr tcod::ColorRGB white{ 255, 255, 255 };
    constexpr tcod::ColorRGB black{ 0, 0, 0 };
    constexpr tcod::ColorRGB light_yellow{ 255, 255, 150 };
    constexpr tcod::ColorRGB green{ 0, 60, 0 };
    constexpr tcod::ColorRGB red{ 255, 60, 0 };
    constexpr tcod::ColorRGB dark_red{ 191, 0, 0 };
    constexpr tcod::ColorRGB desaturated_green{ 63, 127, 63 };
    constexpr tcod::ColorRGB darker_green{ 0, 127, 0 };
    constexpr tcod::ColorRGB light_azure{ 63, 159, 255 };
    constexpr tcod::ColorRGB dark_azure{ 0, 95, 191 };
    constexpr tcod::ColorRGB light_amber{ 255, 207, 63 };
    constexpr tcod::ColorRGB dark_amber{ 191, 143, 0 };
} // namespace tutorial::color

#endif // COLORS_HPP
