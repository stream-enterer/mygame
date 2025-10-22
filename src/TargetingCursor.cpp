#include "TargetingCursor.hpp"

#include "Colors.hpp"
#include "Engine.hpp"
#include "Map.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace tutorial
{
    TargetingCursor::TargetingCursor(Engine& engine, float maxRange) :
        engine_(engine),
        map_(&engine.GetMap()),
        console_(TCOD_console_new(map_->GetWidth(), map_->GetHeight())),
        context_(engine.GetContext()),
        maxRange_(maxRange),
        cursorPos_(engine.GetPlayer()->GetPos()),
        lastCursorPos_{ -1, -1 },
        isInitialized_(false)
    {
        // Save original console colors
        SaveOriginalColors();

        // Highlight valid targeting tiles
        HighlightValidTiles();

        // Initialize cursor at player position
        MoveCursor(cursorPos_);
        Present();

        isInitialized_ = true;
    }

    TargetingCursor::~TargetingCursor()
    {
        // Always restore console state when targeting ends
        RestoreOriginalColors();
        TCOD_console_delete(console_);
    }

    bool TargetingCursor::SelectTile(int* outX, int* outY)
    {
        SDL_Event sdlEvent;

        while (engine_.IsRunning())
        {
            while (SDL_PollEvent(&sdlEvent))
            {
                // Handle quit
                if (sdlEvent.type == SDL_EVENT_QUIT)
                {
                    engine_.Quit();
                    return false;
                }

                // Handle mouse motion
                if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION)
                {
                    HandleMouseMotion(sdlEvent.motion.x, sdlEvent.motion.y);
                }

                // Handle mouse selection
                if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                    && sdlEvent.button.button == SDL_BUTTON_LEFT)
                {
                    if (HandleSelection())
                    {
                        *outX = cursorPos_.x;
                        *outY = cursorPos_.y;
                        return true;
                    }
                }

                // Handle right-click cancellation
                if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                    && sdlEvent.button.button == SDL_BUTTON_RIGHT)
                {
                    return false;
                }

                // Handle keyboard
                if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
                {
                    SDL_Keycode key = sdlEvent.key.key;

                    // Escape cancels
                    if (key == SDLK_ESCAPE)
                    {
                        return false;
                    }

                    // Enter/Space confirms selection
                    if (key == SDLK_RETURN || key == SDLK_SPACE)
                    {
                        if (HandleSelection())
                        {
                            *outX = cursorPos_.x;
                            *outY = cursorPos_.y;
                            return true;
                        }
                    }

                    // Arrow keys move cursor
                    pos_t delta{ 0, 0 };
                    if (key == SDLK_UP)
                        delta = pos_t{ 0, -1 };
                    else if (key == SDLK_DOWN)
                        delta = pos_t{ 0, 1 };
                    else if (key == SDLK_LEFT)
                        delta = pos_t{ -1, 0 };
                    else if (key == SDLK_RIGHT)
                        delta = pos_t{ 1, 0 };

                    if (delta.x != 0 || delta.y != 0)
                    {
                        HandleKeyboardMovement(delta);
                    }
                }
            }
        }

        return false;
    }

    void TargetingCursor::HandleMouseMotion(int mouseX, int mouseY)
    {
        SDL_Window* window = TCOD_context_get_sdl_window(context_);
        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        // Use full console dimensions, not just map dimensions
        int consoleWidth = engine_.GetConfig().width;
        int consoleHeight = engine_.GetConfig().height;

        int tileX = (mouseX * consoleWidth) / windowWidth;
        int tileY = (mouseY * consoleHeight) / windowHeight;

        pos_t requestedPos{ tileX, tileY };

        // Clamp to range if needed
        pos_t clampedPos = ClampToRange(requestedPos);

        // Only update if position actually changed
        if (clampedPos.x != cursorPos_.x || clampedPos.y != cursorPos_.y)
        {
            MoveCursor(clampedPos);
        }
    }

    void TargetingCursor::HandleKeyboardMovement(pos_t delta)
    {
        pos_t requestedPos = cursorPos_ + delta;

        // Check map bounds first
        if (!map_->IsInBounds(requestedPos))
        {
            return;
        }

        // Check if new position is valid target
        if (!IsValidTarget(requestedPos))
        {
            return;
        }

        MoveCursor(requestedPos);
    }

    bool TargetingCursor::HandleSelection()
    {
        // Only allow selection of valid, explored tiles
        return map_->IsExplored(cursorPos_) && IsValidTarget(cursorPos_);
    }

    void TargetingCursor::MoveCursor(pos_t newPos)
    {
        // Erase old cursor position
        if (lastCursorPos_.x >= 0 && lastCursorPos_.y >= 0)
        {
            EraseCursor();
        }

        // Update position
        cursorPos_ = newPos;
        lastCursorPos_ = newPos;

        // Update mouse position state in engine
        engine_.SetMousePos(cursorPos_);

        // Draw new cursor
        DrawCursor();

        Present();
    }

    void TargetingCursor::DrawCursor()
    {
        // Only draw cursor on explored, valid tiles
        if (map_->IsExplored(cursorPos_) && IsValidTarget(cursorPos_))
        {
            TCOD_console_put_rgb(console_, cursorPos_.x, cursorPos_.y, 0, NULL,
                                 &color::white, TCOD_BKGND_SET);
        }
    }

    void TargetingCursor::EraseCursor()
    {
        // Restore to highlighted state (if in FOV and range) or original
        if (map_->IsInFov(lastCursorPos_) && IsValidTarget(lastCursorPos_))
        {
            // Restore to highlighted color
            tcod::ColorRGB col =
                originalColors_[lastCursorPos_.x
                                + lastCursorPos_.y * map_->GetWidth()];
            col.r = std::min(255, static_cast<int>(col.r * 1.2f));
            col.g = std::min(255, static_cast<int>(col.g * 1.2f));
            col.b = std::min(255, static_cast<int>(col.b * 1.2f));
            TCOD_console_put_rgb(console_, lastCursorPos_.x, lastCursorPos_.y,
                                 0, NULL, &col, TCOD_BKGND_SET);
        }
        else if (map_->IsExplored(lastCursorPos_))
        {
            // Restore to original color
            const tcod::ColorRGB& col =
                originalColors_[lastCursorPos_.x
                                + lastCursorPos_.y * map_->GetWidth()];
            TCOD_console_put_rgb(console_, lastCursorPos_.x, lastCursorPos_.y,
                                 0, NULL, &col, TCOD_BKGND_SET);
        }
    }

    bool TargetingCursor::IsValidTarget(pos_t pos) const
    {
        // Infinite range (maxRange_ == 0) means any explored tile is valid
        if (maxRange_ == 0.0f)
        {
            return map_->IsExplored(pos);
        }

        // Otherwise check range from player
        return map_->IsExplored(pos)
               && engine_.GetPlayer()->GetDistance(pos.x, pos.y) <= maxRange_;
    }

    pos_t TargetingCursor::ClampToRange(pos_t pos) const
    {
        // If infinite range or already in range, return as-is
        if (maxRange_ == 0.0f
            || engine_.GetPlayer()->GetDistance(pos.x, pos.y) <= maxRange_)
        {
            return pos;
        }

        // Find closest valid position to requested position
        int bestX = pos.x;
        int bestY = pos.y;
        float bestDist = 1e6f;

        for (int cx = 0; cx < map_->GetWidth(); cx++)
        {
            for (int cy = 0; cy < map_->GetHeight(); cy++)
            {
                if (map_->IsExplored(pos_t{ cx, cy })
                    && engine_.GetPlayer()->GetDistance(cx, cy) <= maxRange_)
                {
                    int dx = cx - pos.x;
                    int dy = cy - pos.y;
                    float dist =
                        std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestX = cx;
                        bestY = cy;
                    }
                }
            }
        }

        return pos_t{ bestX, bestY };
    }

    void TargetingCursor::SaveOriginalColors()
    {
        originalColors_.resize(map_->GetWidth() * map_->GetHeight());

        // First, render complete game state (entities will be drawn on top of
        // map)
        engine_.Render();

        // Now we need to get the console size from engine's config
        int consoleWidth = engine_.GetConfig().width;
        int consoleHeight = engine_.GetConfig().height;

        // Create a temporary console to capture the current rendered state
        TCOD_Console* tempConsole =
            TCOD_console_new(consoleWidth, consoleHeight);

        // The engine just rendered to its console and presented it
        // We need to copy from the engine's console, but we can't access it
        // directly Instead, we'll re-render the map portion to our console
        TCOD_console_clear(console_);
        map_->Render(console_);

        // Render entities in FOV to our console (copying what Engine::Render
        // does)
        const auto& entities = engine_.GetEntities();
        for (const auto& entity : entities)
        {
            const auto pos = entity->GetPos();
            if (map_->IsInFov(pos))
            {
                const auto& renderable = entity->GetRenderable();
                renderable->Render(console_, pos);
            }
        }

        // Save the background colors from the map area only
        for (int cx = 0; cx < map_->GetWidth(); cx++)
        {
            for (int cy = 0; cy < map_->GetHeight(); cy++)
            {
                TCOD_color_t tcodCol =
                    TCOD_console_get_char_background(console_, cx, cy);
                originalColors_[cx + cy * map_->GetWidth()] =
                    tcod::ColorRGB{ tcodCol.r, tcodCol.g, tcodCol.b };
            }
        }

        TCOD_console_delete(tempConsole);
    }

    void TargetingCursor::RestoreOriginalColors()
    {
        // Restore map to console_
        map_->Render(console_);

        // Then trigger full engine render to update everything
        engine_.Render();
    }

    void TargetingCursor::HighlightValidTiles()
    {
        for (int cx = 0; cx < map_->GetWidth(); cx++)
        {
            for (int cy = 0; cy < map_->GetHeight(); cy++)
            {
                if (map_->IsInFov(pos_t{ cx, cy })
                    && IsValidTarget(pos_t{ cx, cy }))
                {
                    tcod::ColorRGB col =
                        originalColors_[cx + cy * map_->GetWidth()];
                    col.r = std::min(255, static_cast<int>(col.r * 1.2f));
                    col.g = std::min(255, static_cast<int>(col.g * 1.2f));
                    col.b = std::min(255, static_cast<int>(col.b * 1.2f));

                    TCOD_console_put_rgb(console_, cx, cy, 0, NULL, &col,
                                         TCOD_BKGND_SET);
                }
            }
        }
    }

    void TargetingCursor::Present()
    {
        // Create a full-size console for presentation that includes UI
        int consoleWidth = engine_.GetConfig().width;
        int consoleHeight = engine_.GetConfig().height;
        TCOD_Console* presentConsole =
            TCOD_console_new(consoleWidth, consoleHeight);

        // Clear the presentation console
        TCOD_console_clear(presentConsole);

        // Blit our map+entities+targeting overlay to the presentation console
        TCOD_console_blit(console_, 0, 0, map_->GetWidth(), map_->GetHeight(),
                          presentConsole, 0, 0, 1.0f, 1.0f);

        // Render UI elements on top using Engine's helper
        engine_.RenderGameUI(presentConsole);

        // Present the complete console
        TCOD_context_present(context_, presentConsole, nullptr);

        TCOD_console_delete(presentConsole);
    }

} // namespace tutorial
