#include "Configuration.hpp"
#include "Engine.hpp"

#include <string>

int main()
{
    static const tutorial::Configuration config { "libtcod C++ tutorial 8", 80,
                                                  50, 60 };

    tutorial::Engine engine { config };

    while (engine.IsRunning())
    {
        engine.GetInput();
        engine.HandleEvents();
        engine.Render();
    }

    return 0;
}
