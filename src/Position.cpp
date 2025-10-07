#include "Position.hpp"

#include <type_traits>

namespace tutorial
{
    static_assert(std::is_standard_layout<pos_t>(), "");
    static_assert(std::is_trivially_copyable<pos_t>(), "");
} // namespace tutorial
