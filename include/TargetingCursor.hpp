#ifndef TARGETING_CURSOR_HPP
#define TARGETING_CURSOR_HPP

#include "Position.hpp"

#include <libtcod.h>
#include <libtcod/color.hpp>

#include <functional>
#include <vector>

namespace tutorial
{
    // Enum for different targeting visualization types
    enum class TargetingType
    {
        None, // No special highlighting (for closest_enemy, self, etc.)
        Beam, // Show beam from player to cursor
        Area  // Show area effect radius around cursor
    };

    class Engine;
    class Map;

    // Encapsulates all targeting cursor logic with proper separation of
    // concerns
    class TargetingCursor
    {
    public:
        TargetingCursor(Engine& engine, float maxRange,
                        TargetingType type = TargetingType::None,
                        float radius = 0.0f);
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

        // Highlighting based on targeting type
        void UpdateHighlights();
        void DrawBeamHighlight();
        void DrawAreaHighlight();

        // Helper methods
        void Present();

        Engine& engine_;
        const Map* map_;
        TCOD_Console* console_;
        TCOD_Context* context_;
        const TCOD_ViewportOptions* viewportOptions_;
        TCOD_Console* engineConsole_;

        float maxRange_;
        float radius_; // For area targeting
        TargetingType targetingType_;
        pos_t cursorPos_;
        pos_t lastCursorPos_;

        // Saved console state for restoration
        std::vector<tcod::ColorRGB> originalColors_;
        bool isInitialized_;

        std::function<bool(int, int)> validator_;
    };

} // namespace tutorial

#endif // TARGETING_CURSOR_HPP
