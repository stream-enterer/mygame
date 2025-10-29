#include "MessageLogWindow.hpp"

#include "Engine.hpp"
#include "MessageLog.hpp"
#include "UiWindow.hpp"

#include <cstring>
#include <map>

namespace tutorial
{
	MessageLogWindow::MessageLogWindow(std::size_t width,
	                                   std::size_t height, pos_t pos,
	                                   const MessageLog& log)
	    : UiWindowBase(width, height, pos), log_(log)
	{
	}

	void MessageLogWindow::Render(TCOD_Console* parent) const
	{
		auto messages = log_.GetMessages();

		TCOD_console_clear(console_);

		int y_offset = TCOD_console_get_height(console_) - 1;

		for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
			auto line_height = TCOD_console_get_height_rect_fmt(
			    console_, 0, 0, TCOD_console_get_width(console_),
			    TCOD_console_get_height(console_), "%s",
			    it->text.c_str());

			if (line_height > 1) {
				y_offset -= line_height - 1;
			}

			{
				auto* line = TCOD_console_new(
				    TCOD_console_get_width(console_),
				    line_height);

				TCOD_console_printn_rect(
				    line, 0, 0, TCOD_console_get_width(line),
				    TCOD_console_get_height(line),
				    it->text
				        .length(), // n = string length in bytes
				    it->text.c_str(), // str
				    NULL,             // fg (use default)
				    NULL,             // bg (use default)
				    TCOD_BKGND_NONE,  // flag
				    TCOD_LEFT);       // alignment

				TCOD_console_blit(
				    line, 0, 0, TCOD_console_get_width(line),
				    TCOD_console_get_height(line), console_, 0,
				    y_offset, 1.0f, 1.0f);

				TCOD_console_delete(line);
			}

			--y_offset;

			if (y_offset < 0) {
				break;
			}
		}

		TCOD_console_blit(console_, 0, 0,
		                  TCOD_console_get_width(console_),
		                  TCOD_console_get_height(console_), parent,
		                  pos_.x, pos_.y, 1.0f, 1.0f);
	}

	void MessageLogWindow::RenderMouseLook(TCOD_Console* parent,
	                                       const Engine& engine) const
	{
		pos_t mousePos = engine.GetMousePos();

		// If mouse is out of FOV, nothing to render
		if (!engine.IsInFov(mousePos)) {
			return;
		}

		// Build comma-separated list of entity names at mouse position
		// Group identical entities and show counts
		char buf[256] = "";
		bool first = true;

		// Map to count entities by name
		std::map<std::string, int> entityCounts;
		std::map<std::string, const Entity*> entityRefs;

		// Scan through all entities IN REVERSE (top to bottom render
		// order)
		const auto& entities = engine.GetEntities();
		for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
			const auto& entity = *it;
			if (entity->GetPos() == mousePos) {
				std::string name = entity->GetName();
				int stackCount = entity->GetStackCount();

				// Accumulate counts
				entityCounts[name] += stackCount;

				// Keep reference for plural name
				if (entityRefs.find(name) == entityRefs.end()) {
					entityRefs[name] = entity.get();
				}
			}
		}

		// Build the display string
		for (const auto& pair : entityCounts) {
			if (!first) {
				strcat(buf, ", ");
			} else {
				first = false;
			}

			const std::string& name = pair.first;
			int count = pair.second;
			const Entity* entity = entityRefs[name];

			if (count > 1) {
				// Use plural name for multiple items
				char countBuf[32];
				snprintf(countBuf, sizeof(countBuf), "%d ",
				         count);
				strcat(buf, countBuf);
				strcat(buf, entity->GetPluralName().c_str());
			} else {
				// Single item, use regular name
				strcat(buf, name.c_str());
			}
		}

		// Display the text at top of parent console
		tcod::ColorRGB textColor = tcod::ColorRGB { 192, 192, 192 };
		TCOD_printf_rgb(
		    parent,
		    (TCOD_PrintParamsRGB) { .x = 1,
		                            .y = 0,
		                            .width = 0,
		                            .height = 0,
		                            .fg = &textColor,
		                            .bg = NULL,
		                            .flag = TCOD_BKGND_NONE,
		                            .alignment = TCOD_LEFT },
		    "%s", buf);
	}
} // namespace tutorial
