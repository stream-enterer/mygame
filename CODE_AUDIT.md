# Code Quality Audit Report

**Date:** 2025-10-26
**Auditor:** Claude Code
**Scope:** Codebase-wide audit for red flags as defined in CLAUDE.md

## Executive Summary

This audit reviewed the codebase for the following red flags from CLAUDE.md:
1. Functions with many local variables → split them
2. **More than 3 indentation levels → extract helpers** (PRIMARY FOCUS)
3. Repeated patterns → extract reusable functions
4. `else` after `return` → remove redundant else
5. Code needs comments to explain WHAT → refactor for clarity
6. Special-case handling → seek uniform logic

**Total Red Flags Found:** 9 major issues across 3 files

**Priority Files:** Engine.cpp, Event.cpp, EventHandler.cpp

---

## Critical Issues (Priority 1)

### 1. Engine.cpp - `Render()` Function (Lines 1043-1169)
**Red Flag:** Repeated patterns + Deep nesting
**Severity:** HIGH
**Lines of Code:** 126 lines

**Problem:**
The rendering code for map + entities + health bar + message log is duplicated 5 times across different window states:
- MainGame (lines 1058-1077)
- Inventory (lines 1080-1099)
- ItemSelection (lines 1100-1119)
- PauseMenu (lines 1120-1142)
- LevelUpMenu (lines 1143-1166)

Each block has 3-4 levels of indentation with nested loops and conditionals.

**Recommendation:**
Extract common rendering logic into helper methods:
```cpp
void Engine::RenderGameBackground();  // Map + entities + health bar + message log
void Engine::RenderMainGame();        // Calls RenderGameBackground()
void Engine::RenderInventoryState();  // Calls RenderGameBackground() + inventory
void Engine::RenderMenuState();       // Calls RenderGameBackground() + menu
```

**Impact:** Would reduce ~100 lines of duplication and flatten indentation

---

### 2. Engine.cpp - `MenuConfirm()` Function (Lines 526-657)
**Red Flag:** Deep nesting + Many branches + Excessive length
**Severity:** HIGH
**Lines of Code:** 131 lines

**Problem:**
- Multiple nested switch statements (4 levels deep in places)
- Handles 4 different menu types in one massive function
- CharacterCreation logic (lines 535-560)
- StartMenu logic (lines 561-586)
- PauseMenu logic (lines 587-603)
- LevelUpMenu logic (lines 604-656)

**Recommendation:**
Split into separate methods:
```cpp
void Engine::HandleCharacterCreationConfirm(MenuAction action);
void Engine::HandleStartMenuConfirm(MenuAction action);
void Engine::HandlePauseMenuConfirm(MenuAction action);
void Engine::HandleLevelUpConfirm(MenuAction action);
```

**Impact:** Would reduce nesting from 4 levels to 2, and split into 4 focused functions

---

### 3. Event.cpp - `DieAction::Execute()` Function (Lines 95-188)
**Red Flag:** Deep nesting + Many local variables + Excessive length
**Severity:** HIGH
**Lines of Code:** 93 lines

**Problem:**
- Lines 117-184: Deeply nested XP and level-up calculation logic (5 levels deep)
- Lines 146-168: Nested while loops calculating XP thresholds
- At least 10 local variables in the XP calculation section
- Level-up logic buried inside death handling

**Recommendation:**
Extract XP/level-up logic:
```cpp
void DestructibleComponent::CalculateLevel(unsigned int xp, unsigned int& outLevel, unsigned int& outXpForLevel);
bool DestructibleComponent::CheckLevelUp(unsigned int oldXp, unsigned int newXp);
void Engine::HandlePlayerLevelUp(unsigned int newLevel);
```

Move XP calculation constants to ConfigManager or a dedicated XPSystem class.

**Impact:** Would reduce nesting from 5 levels to 2, split 93 lines into 3-4 focused functions

---

## High Priority Issues (Priority 2)

### 4. Engine.cpp - `NextLevel()` Function (Lines 801-973)
**Red Flag:** Many local variables + Excessive length + Deep nesting
**Severity:** MEDIUM-HIGH
**Lines of Code:** 172 lines

**Problem:**
- 172 lines handling level transition, inventory saving, entity recreation, UI recreation
- Lines 853-870: Deeply nested inventory extraction (4 levels)
- Many local variables (savedInventory, savedDestructible, savedAttacker, levelConfigPath, etc.)

**Recommendation:**
Extract into helper methods:
```cpp
void Engine::SavePlayerState(PlayerState& outState);
void Engine::ClearCurrentLevel();
void Engine::GenerateNewLevel(const LevelConfig& config);
void Engine::RestorePlayerState(const PlayerState& state);
void Engine::RecreatePlayerUI();
```

**Impact:** Would split 172 lines into 5-6 focused functions, reduce nesting to 2 levels

---

### 5. EventHandler.cpp - Repeated `Dispatch()` Implementations
**Red Flag:** Repeated patterns
**Severity:** MEDIUM-HIGH
**Affected Functions:** 4 nearly identical implementations

**Problem:**
Menu handler `Dispatch()` methods are 95% identical:
- `PauseMenuEventHandler::Dispatch()` (lines 334-376)
- `StartMenuEventHandler::Dispatch()` (lines 384-426)
- `CharacterCreationEventHandler::Dispatch()` (lines 435-477)
- `LevelUpMenuEventHandler::Dispatch()` (lines 485-523)

All handle UP/DOWN/ENTER/ESCAPE with only minor variations.

**Recommendation:**
Create a reusable base implementation with customization points:
```cpp
class MenuEventHandlerBase : public BaseEventHandler {
protected:
    virtual bool AllowEscape() const = 0;
    virtual std::unique_ptr<Command> OnEscape() const = 0;
};
```

Or use a configurable menu handler that takes options.

**Impact:** Would eliminate ~120 lines of duplication

---

## Medium Priority Issues (Priority 3)

### 6. Engine.cpp - Repeated Menu Position Calculation
**Red Flag:** Repeated patterns
**Severity:** MEDIUM
**Affected Lines:** 302-309, 375-388, 936-944

**Problem:**
The pattern for calculating centered menu/window positions is repeated 3+ times:
```cpp
if (cfg.GetInventoryCenterOnScreen()) {
    invPos = pos_t { static_cast<int>(config_.width) / 2 - width / 2,
                     static_cast<int>(config_.height) / 2 - height / 2 };
} else {
    invPos = pos_t { 0, 0 };
}
```

**Recommendation:**
Extract to helper method:
```cpp
pos_t Engine::CalculateWindowPosition(int width, int height, bool center) const;
```

**Impact:** Would eliminate ~30 lines of duplication

---

### 7. Engine.cpp - `GetClosestMonster()` Formatting (Lines 768-789)
**Red Flag:** Inconsistent formatting
**Severity:** LOW
**Affected Lines:** 774-778

**Problem:**
The multi-line condition has inconsistent brace placement:
```cpp
if (entity->GetFaction() == Faction::MONSTER
    && entity->GetDestructible()
    && !entity->GetDestructible()->IsDead())

{  // Opening brace on separate line
```

**Recommendation:**
Move opening brace to same line as condition (per project standards).

**Impact:** Minor formatting consistency fix

---

### 8. EntityManager.cpp - Similar `PlaceItems()` and `PlaceEntities()`
**Red Flag:** Repeated patterns
**Severity:** MEDIUM
**Affected Lines:** 53-96, 130-182

**Problem:**
Both functions follow nearly identical structure:
1. Check spawn chance
2. Get spawn table
3. Loop to spawn N entities
4. Check if position is blocked
5. Roll from spawn table and create entity

**Recommendation:**
Extract common logic:
```cpp
void EntityManager::PlaceFromSpawnTable(
    const Room& room,
    const SpawnConfig& config,
    const SpawnTable* table,
    std::function<bool(pos_t)> isBlocked
);
```

**Impact:** Would eliminate ~50 lines of duplication, improve maintainability

---

### 9. EventHandler.cpp - `BaseEventHandler::Dispatch()` Switch (Lines 183-253)
**Red Flag:** Long switch statement
**Severity:** LOW-MEDIUM
**Affected Lines:** 183-253

**Problem:**
70-line switch statement for SDL key to TCOD key mapping.

**Recommendation:**
Consider using a static lookup table (std::unordered_map) initialized once:
```cpp
static const std::unordered_map<SDL_Keycode, std::pair<TCOD_keycode_t, char>> SDL_TO_TCOD_MAP = {
    {SDLK_UP, {TCODK_UP, '\0'}},
    {SDLK_DOWN, {TCODK_DOWN, '\0'}},
    // ...
};
```

**Impact:** Would reduce from 70 lines to ~30 lines, improve readability

---

## Summary by File

### Engine.cpp (5 issues)
1. `Render()` - Massive duplication (HIGH PRIORITY)
2. `MenuConfirm()` - Deep nesting (HIGH PRIORITY)
3. `NextLevel()` - Long function (MEDIUM-HIGH)
4. Menu position calculation - Repeated (MEDIUM)
5. `GetClosestMonster()` - Formatting (LOW)

### Event.cpp (1 issue)
1. `DieAction::Execute()` - Deep nesting (HIGH PRIORITY)

### EventHandler.cpp (2 issues)
1. Menu `Dispatch()` duplication (MEDIUM-HIGH)
2. `BaseEventHandler::Dispatch()` switch (LOW-MEDIUM)

### EntityManager.cpp (1 issue)
1. `PlaceItems()`/`PlaceEntities()` similarity (MEDIUM)

---

## Recommended Action Plan

### Phase 1: Critical Refactoring (HIGH Priority)
These should be addressed immediately as they violate the "3 indentation levels" guideline significantly:

1. **Engine.cpp - `Render()`**: Extract `RenderGameBackground()` helper
2. **Engine.cpp - `MenuConfirm()`**: Split into 4 menu-specific handlers
3. **Event.cpp - `DieAction::Execute()`**: Extract XP/level-up calculation logic

**Estimated Impact:** ~300 lines reduced, nesting levels cut in half

### Phase 2: High Priority Cleanup (MEDIUM-HIGH)
These improve maintainability and reduce duplication:

4. **Engine.cpp - `NextLevel()`**: Split into state save/restore helpers
5. **EventHandler.cpp - Menu Dispatch()**: Create base implementation

**Estimated Impact:** ~200 lines reduced, improved consistency

### Phase 3: Medium Priority Improvements (MEDIUM)
Nice-to-have improvements for code quality:

6. **Engine.cpp - Menu position calculation**: Extract helper
7. **EntityManager.cpp - PlaceItems/PlaceEntities**: Extract common logic

**Estimated Impact:** ~80 lines reduced

### Phase 4: Low Priority Polish (LOW)
Minor improvements:

8. **Engine.cpp - `GetClosestMonster()`**: Fix brace formatting
9. **EventHandler.cpp - Key mapping**: Consider lookup table

**Estimated Impact:** Improved consistency

---

## Metrics

**Total Lines Affected:** ~700 lines
**Potential Line Reduction:** ~500 lines (if all recommendations implemented)
**Functions Requiring Splitting:** 6 major functions
**Average Indentation Reduction:** 3-4 levels → 2 levels
**Duplication Eliminated:** ~300 lines of repeated code

---

## Notes

- The codebase generally follows good practices (smart pointers, RAII, component pattern)
- Most issues are in Engine.cpp, which has grown to handle too many responsibilities
- Refactoring should maintain existing behavior - comprehensive testing recommended
- Consider extracting UI-related code from Engine into a dedicated UIManager class
- Level transition logic could be moved to a LevelManager class

---

## Conclusion

The codebase has solid architecture but suffers from a few large functions that violate the "3 indentation levels" guideline. The primary issues are in **Engine.cpp**, particularly:

1. The `Render()` function (massive duplication)
2. The `MenuConfirm()` function (deep nesting)
3. The level-up logic in `DieAction::Execute()` (Event.cpp)

Addressing the Phase 1 critical issues would significantly improve code maintainability and adherence to project guidelines.
