#include "TargetingCursor.hpp"

#include "Colors.hpp"
#include "Engine.hpp"
#include "Map.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace tutorial
{
	namespace
	{
		// Helper: Map SDL keycode to movement delta
		// Returns {0, 0} if key is not a movement key
		// Supports arrow keys, numpad cardinal directions, and numpad
		// diagonals
		pos_t GetMovementDelta(SDL_Keycode key)
		{
			switch (key) {
				// Arrow keys
				case SDLK_UP:
					return { 0, -1 };
				case SDLK_DOWN:
					return { 0, 1 };
				case SDLK_LEFT:
					return { -1, 0 };
				case SDLK_RIGHT:
					return { 1, 0 };

				// Numpad cardinal directions
				case SDLK_KP_8:
					return { 0, -1 }; // Up
				case SDLK_KP_2:
					return { 0, 1 }; // Down
				case SDLK_KP_4:
					return { -1, 0 }; // Left
				case SDLK_KP_6:
					return { 1, 0 }; // Right

				// Numpad diagonals
				case SDLK_KP_7:
					return { -1, -1 }; // Up-Left
				case SDLK_KP_9:
					return { 1, -1 }; // Up-Right
				case SDLK_KP_1:
					return { -1, 1 }; // Down-Left
				case SDLK_KP_3:
					return { 1, 1 }; // Down-Right

				default:
					return { 0, 0 };
			}
		}
	} // namespace
	TargetingCursor::TargetingCursor(Engine& engine, float maxRange,
	                                 TargetingType type, float radius)
	    : engine_(engine),
	      map_(&engine.GetMap()),
	      console_(TCOD_console_new(map_->GetWidth(), map_->GetHeight())),
	      context_(engine.GetContext()),
	      viewportOptions_(&engine.GetViewportOptions()),
	      maxRange_(maxRange),
	      radius_(radius),
	      targetingType_(type),
	      cursorPos_(engine.GetPlayer()->GetPos()),
	      lastCursorPos_ { -1, -1 },
	      isInitialized_(false),
	      validator_(nullptr)
	{
		// Save original console colors
		SaveOriginalColors();

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

	bool TargetingCursor::SelectTile(
	    pos_t* outPos, std::function<bool(pos_t)> validator)

	{
		// Store validator for use in HandleSelection
		validator_ = validator;
		SDL_Event sdlEvent;

		while (engine_.IsRunning()) {
			while (SDL_PollEvent(&sdlEvent)) {
				// Handle quit
				if (sdlEvent.type == SDL_EVENT_QUIT) {
					engine_.Quit();
					return false;
				}

				// Handle mouse motion
				if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION) {
					HandleMouseMotion(sdlEvent.motion.x,
					                  sdlEvent.motion.y);
				}

				// Handle mouse selection
				if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
				    && sdlEvent.button.button
				           == SDL_BUTTON_LEFT) {
					if (HandleSelection()) {
						*outPos = cursorPos_;
						return true;
					}
				}

				// Handle right-click cancellation
				if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
				    && sdlEvent.button.button
				           == SDL_BUTTON_RIGHT) {
					return false;
				}

				// Handle keyboard
				if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
					SDL_Keycode key = sdlEvent.key.key;

					// Escape cancels
					if (key == SDLK_ESCAPE) {
						return false;
					}

					// Enter/Space confirms selection
					if (key == SDLK_RETURN
					    || key == SDLK_SPACE) {
						if (HandleSelection()) {
							*outPos = cursorPos_;
							return true;
						}
					}

					// Check for movement keys (arrows, numpad,
					// diagonals)
					pos_t delta = GetMovementDelta(key);
					if (delta.x != 0 || delta.y != 0) {
						HandleKeyboardMovement(delta);
					}
				}
			}
		}

		return false;
	}

	void TargetingCursor::HandleMouseMotion(int mouseX, int mouseY)
	{
		// Use libtcod's coordinate conversion which handles viewport
		// transforms
		int tileX = mouseX;
		int tileY = mouseY;
		TCOD_context_screen_pixel_to_tile_i(context_, &tileX, &tileY);

		pos_t requestedPos { tileX, tileY };

		// No more clamping - cursor moves freely
		// Only update if position actually changed
		if (requestedPos.x != cursorPos_.x
		    || requestedPos.y != cursorPos_.y) {
			MoveCursor(requestedPos);
		}
	}

	void TargetingCursor::HandleKeyboardMovement(pos_t delta)
	{
		pos_t requestedPos = cursorPos_ + delta;

		// Check map bounds only
		if (!map_->IsInBounds(requestedPos)) {
			return;
		}

		MoveCursor(requestedPos);
	}

	bool TargetingCursor::HandleSelection()
	{
		// TargetingCursor only validates UI constraints (explored +
		// range) IsValidTarget already checks both explored and range
		if (!IsValidTarget(cursorPos_)) {
			return false;
		}

		// Validator handles game logic (entity presence, LOS, etc.)
		if (validator_ && !validator_(cursorPos_)) {
			// Validation failed - stay in targeting mode
			// (validator has already logged the message)
			// Re-render to show the message immediately
			Present();
			return false;
		}

		return true;
	}

	void TargetingCursor::MoveCursor(pos_t newPos)
	{
		// Clear all previous highlights
		if (lastCursorPos_.x >= 0 && lastCursorPos_.y >= 0) {
			// Restore original colors for the entire map (including
			// unexplored tiles)
			for (int x = 0; x < map_->GetWidth(); ++x) {
				for (int y = 0; y < map_->GetHeight(); ++y) {
					const tcod::ColorRGB& col =
					    originalColors_
					        [x + y * map_->GetWidth()];
					TCOD_console_put_rgb(console_, x, y, 0,
					                     NULL, &col,
					                     TCOD_BKGND_SET);
				}
			}
		}

		// Update position
		cursorPos_ = newPos;
		lastCursorPos_ = newPos;

		// Update mouse position state in engine
		engine_.SetMousePos(cursorPos_);

		// Draw new highlights and cursor
		UpdateHighlights();

		Present();
	}

	void TargetingCursor::DrawCursor()
	{
		// Always draw cursor, even in unexplored areas
		tcod::ColorRGB cursorColor;
		if (IsValidTarget(cursorPos_)) {
			cursorColor = color::white;
		} else {
			cursorColor = color::red; // Red for invalid
		}
		TCOD_console_put_rgb(console_, cursorPos_.x, cursorPos_.y, 0,
		                     NULL, &cursorColor, TCOD_BKGND_SET);
	}

	bool TargetingCursor::IsValidTarget(pos_t pos) const
	{
		// Must be in FOV (line of sight) to be a valid target
		if (!map_->IsInFov(pos)) {
			return false;
		}

		// Infinite range (maxRange_ == 0) means any tile in FOV is
		// valid
		if (maxRange_ == 0.0f) {
			return true;
		}

		// Otherwise check range from player
		return engine_.GetPlayer()->GetDistance(pos.x, pos.y)
		       <= maxRange_;
	}

	void TargetingCursor::SaveOriginalColors()
	{
		originalColors_.resize(map_->GetWidth() * map_->GetHeight());

		// First, render complete game state (entities will be drawn on
		// top of map)
		engine_.Render();

		// Now we need to get the console size from engine's config
		int consoleWidth = engine_.GetConfig().width;
		int consoleHeight = engine_.GetConfig().height;

		// Create a temporary console to capture the current rendered
		// state
		TCOD_Console* tempConsole =
		    TCOD_console_new(consoleWidth, consoleHeight);

		// The engine just rendered to its console and presented it
		// We need to copy from the engine's console, but we can't
		// access it directly Instead, we'll re-render the map portion
		// to our console
		TCOD_console_clear(console_);
		map_->Render(console_);

		// Render entities in FOV to our console (copying what
		// Engine::Render does)
		const auto& entities = engine_.GetEntities();
		for (const auto& entity : entities) {
			const auto pos = entity->GetPos();
			if (map_->IsInFov(pos)) {
				const auto& renderable =
				    entity->GetRenderable();
				renderable->Render(console_, pos);
			}
		}

		// Save the background colors from the map area only
		for (int cx = 0; cx < map_->GetWidth(); cx++) {
			for (int cy = 0; cy < map_->GetHeight(); cy++) {
				TCOD_color_t tcodCol =
				    TCOD_console_get_char_background(console_,
				                                     cx, cy);
				originalColors_[cx + cy * map_->GetWidth()] =
				    tcod::ColorRGB { tcodCol.r, tcodCol.g,
					             tcodCol.b };
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

	void TargetingCursor::Present()
	{
		// Create a full-size console for presentation that includes UI
		int consoleWidth = engine_.GetConfig().width;
		int consoleHeight = engine_.GetConfig().height;
		TCOD_Console* presentConsole =
		    TCOD_console_new(consoleWidth, consoleHeight);

		// Clear the presentation console
		TCOD_console_clear(presentConsole);

		// Blit our map+entities+targeting overlay to the presentation
		// console
		TCOD_console_blit(console_, 0, 0, map_->GetWidth(),
		                  map_->GetHeight(), presentConsole, 0, 0, 1.0f,
		                  1.0f);

		// Render UI elements on top using Engine's helper
		engine_.RenderGameUI(presentConsole);

		// Present the complete console
		TCOD_context_present(context_, presentConsole,
		                     viewportOptions_);

		TCOD_console_delete(presentConsole);
	}

	void TargetingCursor::UpdateHighlights()
	{
		// Only draw highlights if cursor is on a valid target
		if (!IsValidTarget(cursorPos_)) {
			DrawCursor(); // Just draw red cursor
			return;
		}

		// Draw highlights based on targeting type
		switch (targetingType_) {
			case TargetingType::Beam:
				DrawBeamHighlight();
				break;
			case TargetingType::Area:
				DrawAreaHighlight();
				break;
			case TargetingType::None:
			default:
				// No special highlighting, just draw cursor
				break;
		}

		// Always draw cursor on top
		DrawCursor();
	}

	void TargetingCursor::DrawBeamHighlight()
	{
		pos_t playerPos = engine_.GetPlayer()->GetPos();
		tcod::BresenhamLine line({ playerPos.x, playerPos.y },
		                         { cursorPos_.x, cursorPos_.y });

		for (auto it = line.begin(); it != line.end(); ++it) {
			auto [x, y] = *it;

			// Skip player position
			if (x == playerPos.x && y == playerPos.y) continue;

			// Skip cursor position (will be drawn white)
			if (x == cursorPos_.x && y == cursorPos_.y) continue;

			// Yellow highlight for entire beam path (through walls
			// and floors)
			TCOD_console_put_rgb(console_, x, y, 0, NULL,
			                     &color::light_yellow,
			                     TCOD_BKGND_SET);
		}
	}
	void TargetingCursor::DrawAreaHighlight()
	{
		// Highlight all tiles within radius
		for (int dx = -static_cast<int>(radius_);
		     dx <= static_cast<int>(radius_); ++dx) {
			for (int dy = -static_cast<int>(radius_);
			     dy <= static_cast<int>(radius_); ++dy) {
				pos_t checkPos { cursorPos_.x + dx,
					         cursorPos_.y + dy };

				// Check if within radius and map bounds
				if (!map_->IsInBounds(checkPos)) continue;

				float dist = std::sqrt(
				    static_cast<float>(dx * dx + dy * dy));
				if (dist > radius_) continue;

				// Only show for explored tiles
				if (!map_->IsExplored(checkPos)) continue;

				// Skip cursor position (will be drawn white)
				if (checkPos.x == cursorPos_.x
				    && checkPos.y == cursorPos_.y)
					continue;

				// Yellow highlight for area
				TCOD_console_put_rgb(
				    console_, checkPos.x, checkPos.y, 0, NULL,
				    &color::light_yellow, TCOD_BKGND_SET);
			}
		}
	}
} // namespace tutorial
