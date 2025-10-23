#ifndef TARGETING_CURSOR_HPP
#define TARGETING_CURSOR_HPP

#include "Position.hpp"

#include <libtcod.h>
#include <libtcod/color.hpp>

#include <functional>
#include <vector>

namespace tutorial
{
    class Engine;
    class Map;

    // Encapsulates all targeting cursor logic with proper separation of
    // concerns
    class TargetingCursor
    {
    public:
        TargetingCursor(Engine& engine, float maxRange);
        ~TargetingCursor();

        // Main interface - returns true if tile selected, false if cancelled
        bool SelectTile(int* outX, int* outY,
                        std::function<bool(int, int)> validator = nullptr);

    private:
        // Input handling
        void HandleMouseMotion(int mouseX, int mouseY);
        void HandleKeyboardMovement(pos_t delta);
        bool HandleSelection();
        bool HandleCancellation();

        // Cursor state management
        void MoveCursor(pos_t newPos);
        void DrawCursor();
        void EraseCursor();

        // Range validation
        bool IsValidTarget(pos_t pos) const;
        pos_t ClampToRange(pos_t pos) const;

        // Visual state management
        void SaveOriginalColors();
        void RestoreOriginalColors();
        void HighlightValidTiles();

        // Helper methods
        void Present();

        // Preview methods for different targeting types
        void DrawAreaPreview(pos_t center, float radius);
        void DrawBeamPreview(pos_t origin, pos_t target, float range);

        Engine& engine_;
        const Map* map_;
        TCOD_Console* console_;
        TCOD_Context* context_;
        const TCOD_ViewportOptions* viewportOptions_;
        TCOD_Console* engineConsole_;

        float maxRange_;
        pos_t cursorPos_;
        pos_t lastCursorPos_;

        // Saved console state for restoration
        std::vector<tcod::ColorRGB> originalColors_;
        bool isInitialized_;

        std::function<bool(int, int)> validator_;
    };

} // namespace tutorial

#endif // TARGETING_CURSOR_HPP
