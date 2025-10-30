# Roguelike Project Coding Standards

## Pre-Flight Knowledge

**Read this before coding. These are invariant principles.**

### JSON vs C++ Boundary
- **Data goes in JSON**: Stats, colors, spawn weights, combinations of existing behaviors
- **Logic goes in C++**: New game mechanics, new component types, new systems, formulas
- **Decision test**: "Is this just different numbers/settings?" → JSON. "Is this new behavior?" → C++

### Code Style (Enforce Strictly)
- **8-space indentation using actual tab characters** (not spaces)
- **Opening brace on same line**: `void Foo() {`
- **Pointer declarations**: `Type* ptr` (asterisk with type)
- **Naming conventions**:
  - camelCase for methods: `GetHealth()`, `ComputeDamage()`
  - Private members get trailing underscore: `hp_`, `pos_`, `data_`
- **#ifndef header guards** (not #pragma once)
- **Position type**: Always use `pos_t`, never separate `int x, int y` parameters

### Memory Ownership Model
- **std::unique_ptr** = ownership. The holder is responsible for lifetime.
- **Raw pointers** = observation only. Never delete, never assume lifetime.
- **Ownership transfers** = `std::move()`. Make it explicit.
- **Entity ownership**:
  - EntityManager owns entities in the world
  - Player owns entities in inventory
  - Engine holds raw pointers (player_, stairs_) to entities owned by EntityManager
  - Events/Actions never own entities, only hold references or raw pointers

### RAII Principles
- No manual `new`/`delete` ever
- Containers own their contents (vector<unique_ptr<Entity>>)
- Defer entity removal during event processing (mark for removal, delete after turn)
- When removing entities, nullify any raw pointers to them (especially player_)

---

## Search-First Protocol

**Execute during implementation. Prevent reinventing patterns.**

### Before Implementing Any Feature

1. **Search for similar existing implementations**
```bash
   # Examples:
   grep -r "class.*Action.*Event" include/     # Find action types
   grep -r "TemplateRegistry.*Create" src/     # Find entity creation
   grep -r "GetComponent\|GetAttacker" src/    # Find component access
```

2. **If pattern exists**: Match it exactly. Consistency over cleverness.

3. **If pattern doesn't exist**: Implement cleanly, but explicitly call out: "This is a NEW pattern: [description]"
   - This lets user review for potential duplication
   - Future implementations will find and reuse this pattern

### State Awareness

**Working State = Project Knowledge Baseline + All Code Changes From Current Chat**

- **Default assumption**: User implements each step before proceeding to next
- **At each step**: State baseline explicitly
  - Example: "Working from Engine.cpp as modified in Step 3..."
- **Divergence signals** (mental model is wrong):
  - Compile error → STOP, ask user to re-upload affected files
  - User mentions manual changes → STOP, ask for re-upload  
  - Same file modified >5 steps ago → verify state with user
  - User says "check current state" → ask for re-upload
- **After re-upload**: New baseline = re-uploaded state + subsequent chat changes

---

## Pre-Submission Checklist

**Execute before showing code to user. Mechanical verification.**

### Conditional Checks
- [ ] **IF Engine.cpp constructor modified** → verify initializer order matches Engine.hpp declaration order exactly (line-by-line comparison)
- [ ] **IF std::make_unique appears anywhere** → verify `<` follows immediately in ALL instances (search whole response)
- [ ] **IF same file modified >5 steps ago** → verify current state (search project knowledge or ask user)
- [ ] **IF multiple changes in same file separated by <5 lines** → combine into single code block

### Always-On Checks
- [ ] **Nesting depth**: Count indentation in ALL code shown (new AND modified). If ANY function >3 levels → extract helper function NOW
- [ ] **Includes verification**: For EACH file modified:
  - List all types/classes used in that file
  - Verify corresponding #include exists
  - If uncertain, search codebase for where type is declared
- [ ] **Style compliance**:
  - Tabs not spaces (8-space tabs)
  - Trailing underscores on private members
  - Braces on same line
  - pos_t instead of int x, int y

---

## Quick Reference Commands
```bash
# Find event/action patterns
grep -r ": public Action\|: public Event" include/

# Find entity creation patterns  
grep -r "TemplateRegistry.*Create\|entities_\.Spawn" src/

# Find component access patterns
grep -r "GetAttacker\|GetDestructible\|GetComponent" src/

# Find entity lifecycle operations
grep -r "Spawn\|Remove\|entitiesToRemove_" src/

# Find where a type is declared
grep -r "class TypeName\|struct TypeName" include/
```
