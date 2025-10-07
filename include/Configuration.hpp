#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>

namespace tutorial
{
    struct Configuration
    {
        std::string title;
        unsigned int width;
        unsigned int height;
        unsigned int fps;
    };
} // namespace tutorial

#endif // CONFIGURATION_HPP
