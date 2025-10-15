#ifndef RENDER_LAYER_HPP
#define RENDER_LAYER_HPP

namespace tutorial
{
    // Explicit rendering layers for proper overlap handling
    // Lower values render first (bottom), higher values render last (top)
    enum class RenderLayer : int
    {
        CORPSES = 0, // Dead entities, floor decoration
        ITEMS = 10,  // Pickable items on ground
        ACTORS = 20, // Living monsters and NPCs
        PLAYER = 30, // Player character (always on top)
        EFFECTS = 40 // Visual effects (reserved for future use)
    };
} // namespace tutorial

#endif // RENDER_LAYER_HPP
