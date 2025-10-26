# Roguelike Project Coding Standards

## Architecture Overview
**Modern C++ roguelike using libtcod with component-based entity system**

**Core Systems:**
- **Entities**: Game objects with component composition (BaseEntity, Npc, Player)
- **Components**: Modular functionality (AttackerComponent, DestructibleComponent, IconRenderable, AiComponent, Item)
- **Events/Actions**: Command pattern objects with Execute() method (MoveAction, MeleeAction, PickupAction, UseItemAction)
- **Factories**: TemplateRegistry loads JSON entity definitions and creates entities via templates
- **Memory**: std::unique_ptr for ownership, raw pointers for observation only (no manual new/delete)

**Key References:**
- libtcod version: Uses C API (TCOD_Console*) with modern C++ wrappers (tcod::ColorRGB)
- Interface declarations (Map.hpp, Engine.hpp, Entity.hpp) define core APIs
- Existing code shows established patterns - search before implementing

**Data-Driven Design Philosophy:**

The codebase separates data from logic:

**JSON Templates (data/entities/, data/levels/):**
- Entity stats (HP, damage, defense, colors, icons)
- Spawn tables and weights
- Item effects using standard effect types
- Level generation parameters
- **Guideline**: If it's a number, string, color, or combination of existing behaviors → JSON

**C++ Code (src/, include/):**
- Game logic and rules (combat formulas, FOV, pathfinding)
- New component types
- New effect/targeting behaviors
- System architecture
- **Guideline**: If it's new logic or a new system → C++ code

**Decision Criteria**: 
- Adding a stronger orc? → JSON template with higher stats
- Adding poison damage over time? → New C++ Effect subclass + JSON config
- Changing spawn rates? → JSON spawn weights
- Changing how spawning works? → C++ EntityManager code

---

## Code Quality Principles

**Simplicity & Clarity (Max Priority)**
- Limit indentation to 3 levels - beyond that, extract helper functions
- Keep functions focused and short (24-48 lines ideal)
- Use early returns (guard clauses) to flatten nested logic
- Eliminate special-case handling where possible - prefer uniform logic
- Simple, obvious solutions beat clever ones

**Naming**
- Descriptive for functions/classes/globals: `ComputeFOV`, `TemplateRegistry`
- Short for local scope: `i`, `j`, `tmp`, `result`
- Private members get trailing underscore: `hp_`, `pos_`, `data_`
- camelCase for methods: `GetHealth()`, `TakeDamage()`

**Comments**
- Code explains WHAT it does (via clear structure/naming)
- Comments explain WHY: reasoning, gotchas, business rules, non-obvious decisions
- Use named constants instead of magic numbers

**Modern C++ (C++17/20)**
- Smart pointers and RAII everywhere
- Value semantics where appropriate (pos_t is a value type with operator overloads)
- No manual memory management
- Component composition over inheritance
- Data-driven design via JSON templates

---

## Formatting Standards
```cpp
8-space indentation (actual tab characters)
Opening brace same line: void foo() {
Pointer declarations: Type* ptr
#ifndef header guards
```

---

## Concrete Architecture Patterns

### 1. **Event System Pattern**

Events inherit from `Event` base class with `Execute()` method. Events are NOT messages - they are imperative commands that execute immediately when processed.

**Event Hierarchy:**
```cpp
Event (base)
├── EngineEvent (engine state changes)
│   ├── MessageHistoryEvent
│   ├── InventoryEvent
│   ├── NewGameEvent
│   ├── ReturnToGameEvent
│   └── QuitEvent
└── Action (entity behaviors)
    ├── AiAction
    ├── DieAction
    ├── WaitAction
    ├── PickupAction
    ├── UseItemAction
    ├── DropItemAction
    └── DirectionalAction (base for movement/combat)
        ├── BumpAction
        ├── MoveAction
        └── MeleeAction
```

**Event Execution Model:**
- Events are stored in `std::deque<std::unique_ptr<Event>>` in Engine
- Processed synchronously by `Engine::HandleEvents()`
- Events execute immediately, then are destroyed
- Actions can create new events (e.g., BumpAction creates MeleeAction or MoveAction)

**Event Queue Priority:**
- Use `AddEventFront()` for immediate reactions (AI responses during enemy turn)
- Standard pattern: events process front-to-back (FIFO)
- Enemy AI events are queued all at once, then processed in sequence

**Example - Creating and queuing an event:**
```cpp
// From Event.cpp - BumpAction executes sub-actions directly
void BumpAction::Execute() {
    Action::Execute();
    
    if (entity_.GetDestructible()->IsDead()) {
        return;
    }
    
    auto targetPos = entity_.GetPos() + pos_;
    
    // Execute the resolved action directly instead of queueing it
    if (engine_.GetBlockingEntity(targetPos)) {
        MeleeAction(engine_, entity_, pos_).Execute();
    } else {
        MoveAction(engine_, entity_, pos_).Execute();
    }
}
```

**Example - Event queue processing:**
```cpp
// From TurnManager.cpp
void TurnManager::ProcessCommand(std::unique_ptr<Command> command, Engine& engine) {
    if (!command) {
        return;
    }
    
    // Execute the player's command
    command->Execute(engine);
    
    // Process any events created by the command
    engine.HandleEvents();
    
    // If the command consumed a turn, let enemies act
    if (command->ConsumesTurn()) {
        ProcessEnemyTurn(engine);
    }
}

void TurnManager::ProcessEnemyTurn(Engine& engine) {
    // Queue up enemy actions
    const auto& entities = engine.GetEntities();
    
    for (auto& entity : entities) {
        if (engine.IsPlayer(*entity.get())) {
            continue;
        }
        
        if (!entity->CanAct()) {
            continue;
        }
        
        std::unique_ptr<Event> event = 
            std::make_unique<AiAction>(engine, *entity);
        engine.AddEventFront(event);
    }
    
    // Process all enemy actions
    engine.HandleEvents();
}
```

---

### 2. **Entity Factory Pattern (TemplateRegistry)**

Entities are created from JSON templates via `TemplateRegistry`, a singleton that loads entity definitions from `data/entities/` directory.

**Factory Usage:**
```cpp
// Load templates at startup (from Engine.cpp)
TemplateRegistry::Instance().Clear();
TemplateRegistry::Instance().LoadFromDirectory("data/entities");

// Create entity from template
auto entity = TemplateRegistry::Instance().Create("orc", pos_t{10, 10});

// Check if template exists
if (TemplateRegistry::Instance().Has("health_potion")) {
    // ...
}
```

**Template Structure (JSON):**
```json
{
    "orc": {
        "name": "Orc",
        "char": "o",
        "color": [63, 127, 63],
        "blocks": true,
        "faction": "monster",
        "hp": 10,
        "maxHp": 10,
        "defense": 0,
        "power": 3,
        "xpReward": 35,
        "ai": "hostile",
        "spawns": [
            {"location": "dungeon_1", "weight": 80}
        ]
    }
}
```

**Template Processing (from EntityTemplate.cpp):**
```cpp
std::unique_ptr<Entity> EntityTemplate::CreateEntity(pos_t pos) const {
    // Determine faction enum
    Faction factionEnum;
    if (faction == "player") {
        factionEnum = Faction::PLAYER;
    } else if (faction == "monster") {
        factionEnum = Faction::MONSTER;
    } else {
        factionEnum = Faction::NEUTRAL;
    }
    
    // Create appropriate entity type
    if (factionEnum == Faction::PLAYER) {
        return std::make_unique<Player>(
            pos, name, blocks,
            AttackerComponent{static_cast<unsigned int>(power)},
            DestructibleComponent{static_cast<unsigned int>(defense),
                                 static_cast<unsigned int>(maxHp),
                                 static_cast<unsigned int>(hp)},
            IconRenderable{color, icon}, factionEnum, pickable);
    } else if (aiComponent != nullptr) {
        // Monster with AI
        return std::make_unique<Npc>(
            pos, name, blocks, attacker, destructible, 
            renderable, factionEnum, std::move(aiComponent), pickable);
    } else {
        // Item or neutral entity
        return std::make_unique<BaseEntity>(
            pos, name, blocks, attacker, destructible, renderable,
            factionEnum, std::move(itemComponent), pickable);
    }
}
```

---

### 3. **Component Pattern**

All entities have components for modular functionality. Components are NOT ECS-style - they're embedded in entity classes.

**Component Architecture:**
```cpp
// All entities have these components (from Entity.hpp)
class BaseEntity : public Entity {
public:
    BaseEntity(pos_t pos, const std::string& name, bool blocker,
               AttackerComponent attack,
               const DestructibleComponent& defense,
               const IconRenderable& renderable, Faction faction,
               std::unique_ptr<Item> item = nullptr, bool pickable = true,
               bool isCorpse = false);
    
    // Component access - returns nullptr if not present
    AttackerComponent* GetAttacker() const override { return attacker_.get(); }
    DestructibleComponent* GetDestructible() const override { return destructible_.get(); }
    Item* GetItem() const override { return item_.get(); }
    
private:
    std::unique_ptr<AttackerComponent> attacker_;
    std::unique_ptr<DestructibleComponent> destructible_;
    std::unique_ptr<IconRenderable> renderable_;
    std::unique_ptr<Item> item_;  // Optional - only for pickable items
};
```

**Component Usage Examples:**
```cpp
// From Event.cpp - MeleeAction uses components
void MeleeAction::Execute() {
    if (!entity_.GetAttacker()) {
        return;  // Early return if no attack component
    }
    
    auto targetPos = entity_.GetPos() + pos_;
    auto* target = engine_.GetBlockingEntity(targetPos);
    
    if (target && !target->GetDestructible()->IsDead()) {
        auto* attacker = entity_.GetAttacker();
        auto* defender = target->GetDestructible();
        auto damage = attacker->Attack() - defender->GetDefense();
        
        if (damage > 0) {
            engine_.DealDamage(*target, damage);
        }
    }
}
```

**Component Null-Safety Helpers:**
```cpp
// From Entity.hpp - use when component MUST exist
AttackerComponent& RequireAttacker() const {
    auto* component = GetAttacker();
    if (!component) {
        throw std::runtime_error("Entity " + GetName() 
                               + " requires Attacker component");
    }
    return *component;
}
```

**Component Communication Rules:**

Components don't communicate directly with each other. The Entity class acts as the mediator for intra-entity communication, and the Engine class coordinates cross-entity interactions.

**Intra-Entity Communication (Entity is mediator):**
```cpp
// Components accessed through Entity interface
// Entity methods coordinate between components
class Entity {
    virtual void Die() = 0;  // Coordinates Renderable + Destructible changes
    virtual void Use(Engine& engine) = 0;  // Coordinates Item + other components
};
```

**Cross-Entity Communication (Engine is coordinator):**
```cpp
// From Engine.cpp - Engine coordinates component interactions across entities
void Engine::DealDamage(Entity& target, int damage) {
    // Engine retrieves component from target
    auto* destructible = target.GetDestructible();
    if (!destructible) {
        return;
    }
    
    // Engine applies damage through component
    destructible->TakeDamage(damage);
    
    // Engine checks result and triggers follow-up actions
    if (destructible->IsDead()) {
        HandleDeathEvent(target);  // Engine coordinates death handling
    }
}

// Pattern: Engine methods are the "glue" between components
// Components expose data and simple operations
// Engine contains the game logic that combines them
```

**Component State Changes Trigger Engine Actions:**
```cpp
// From Event.cpp - Action notices component state change, notifies Engine
void DieAction::Execute() {
    Action::Execute();
    
    entity_.Die();  // Entity coordinates its own component changes
    
    // But for XP rewards, Engine must coordinate player XP gain
    if (!engine_.IsPlayer(entity_)) {
        auto* destructible = entity_.GetDestructible();
        if (destructible && destructible->GetXpReward() > 0) {
            auto& playerDest = engine_.GetPlayer().RequireDestructible();
            unsigned int xpGained = destructible->GetXpReward();
            playerDest.AddXp(xpGained);
            
            // Engine handles level-up logic
            // ... (level up menu, stat increases)
        }
    }
    
    engine_.HandleDeathEvent(entity_);  // Engine handles corpse spawning
}
```

**Summary - Component Communication Pattern:**
1. **Components** expose data and simple operations (`TakeDamage()`, `Attack()`)
2. **Entity** coordinates its own components (`Die()` updates multiple components)
3. **Engine** coordinates cross-entity logic (`DealDamage()`, `HandleDeathEvent()`)
4. **Actions/Events** trigger state changes and notify Engine of important events

---

### 4. **Type Usage Patterns**

**Position Type:**
```cpp
// Position is a value type (from Position.hpp)
struct pos_t {
    int x;
    int y;
};

// Usage examples
pos_t playerPos{10, 15};
pos_t delta{0, -1};  // Up direction
pos_t newPos = playerPos + delta;  // Operator overloaded

// NOT: int x, int y as separate parameters
// YES: pos_t pos as single parameter
```

**Color Type:**
```cpp
// Use libtcod's modern C++ type (from Components.hpp)
tcod::ColorRGB color{255, 0, 0};  // Red

// NOT: TCODColor (deprecated)
// NOT: TCOD_color_t (C API)
```

**Console Type:**
```cpp
// Use C API pointer type for function parameters
void Render(TCOD_Console* console, pos_t pos) const;

// NOT: TCODConsole (C++ wrapper)
// NOT: tcod::Console (modern wrapper)
```

---

### 5. **Error Handling Strategy**

The codebase uses **exceptions for fatal errors** and **return values/nullptr for recoverable failures**.

**Exception Usage:**
```cpp
// From EntityTemplate.cpp - validation errors throw
EntityTemplate EntityTemplate::FromJson(const std::string& id, const json& j) {
    if (!j.contains("name")) {
        throw std::runtime_error("Entity '" + id 
                               + "' missing required field 'name'");
    }
    // ... more validation
}

// From TemplateRegistry.cpp - missing template throws
std::unique_ptr<Entity> TemplateRegistry::Create(const std::string& id, 
                                                 pos_t pos) const {
    const EntityTemplate* tpl = Get(id);
    if (!tpl) {
        throw std::runtime_error("Template not found: " + id);
    }
    return tpl->CreateEntity(pos);
}
```

**nullptr for Optional Results:**
```cpp
// From TemplateRegistry.cpp - returns nullptr if not found
const EntityTemplate* TemplateRegistry::Get(const std::string& id) const {
    auto it = templates_.find(id);
    if (it != templates_.end()) {
        return &it->second;
    }
    return nullptr;
}

// Usage
const EntityTemplate* tpl = TemplateRegistry::Instance().Get("orc");
if (tpl) {
    // Use template
}
```

**std::optional for Optional Values:**
```cpp
// From EntityTemplate.hpp
struct EntityTemplate {
    std::optional<std::string> aiType;  // Not all entities have AI
    std::optional<ItemData> item;       // Not all entities are items
};
```

**Error Logging (Don't Crash):**
```cpp
// From Engine.cpp - log errors but continue
try {
    TemplateRegistry::Instance().LoadFromDirectory("data/entities");
} catch (const std::exception& e) {
    std::cerr << "[Engine] FATAL: Failed to load entity templates: " 
              << e.what() << std::endl;
    throw;  // Re-throw if truly fatal
}
```

---

### 6. **Entity Lifecycle and Ownership**

Understanding entity ownership prevents memory leaks and dangling pointers.

**Creation and Spawning:**
```cpp
// From Engine.cpp - EntityManager owns all world entities
auto entity = TemplateRegistry::Instance().Create("orc", pos_t{10, 10});
auto& spawnedEntity = entities_.Spawn(std::move(entity));  // Ownership transferred

// entities_ is an EntityManager containing:
// std::vector<std::unique_ptr<Entity>> entities_;
```

**Deferred Removal (Prevents Iterator Invalidation):**
```cpp
// From Engine.cpp - mark for removal, don't remove immediately
void Engine::HandleDeathEvent(Entity& entity) {
    // Mark for deferred removal - safe during event processing
    entitiesToRemove_.push_back(&entity);  // Raw pointer for lookup
    
    if (this->IsPlayer(entity)) {
        eventHandler_ = std::make_unique<GameOverEventHandler>(*this);
        eventQueue_.clear();
        gameOver_ = true;
    }
}

// From Engine.cpp - actually remove after turn completes
void Engine::ProcessDeferredRemovals() {
    for (Entity* entity : entitiesToRemove_) {
        std::string corpseName = "remains of " + entity->GetName();
        pos_t corpsePos = entity->GetPos();
        
        // Create corpse before removing entity
        auto corpse = TemplateRegistry::Instance().Create("corpse", corpsePos);
        if (corpse) {
            corpse->SetRenderPriority(-1);
            SpawnEntity(std::move(corpse), corpsePos);
        }
        
        // CRITICAL: Nullify player pointer if removing player
        if (entity == player_) {
            player_ = nullptr;
        }
        
        // Now safe to remove the entity
        RemoveEntity(entity);
    }
    
    entitiesToRemove_.clear();
}
```

**Ownership Transfer (Pickup/Drop):**
```cpp
// From EntityManager.cpp - Extract entity from world
std::unique_ptr<Entity> EntityManager::Remove(Entity* entity) {
    for (auto it = entities_.begin(); it != entities_.end(); ++it) {
        if (it->get() == entity) {
            auto removed = std::move(*it);  // Extract ownership
            entities_.erase(it);
            return removed;  // Caller now owns
        }
    }
    return nullptr;
}

// From Entity.cpp (Player class) - Transfer ownership to inventory
bool Player::AddToInventory(std::unique_ptr<Entity> item) {
    if (!item) {
        return false;
    }
    inventory_.push_back(std::move(item));  // Player now owns
    return true;
}

// Complete pickup flow:
// 1. Item exists in world (EntityManager owns)
// 2. Player picks up: auto item = entities_.Remove(itemEntity)
// 3. Transfer: player->AddToInventory(std::move(item))
// 4. Item now in inventory_ (Player owns, not EntityManager)
```

**Ownership Patterns Summary:**
- **EntityManager**: Owns all entities in the world (map, monsters, items on ground)
- **Player**: Owns entities in inventory
- **Engine**: Holds raw pointers (player_, stairs_) to entities owned by EntityManager
- **Events/Actions**: Never own entities, only hold references or raw pointers
- **Deferred Removal**: Mark with raw pointer, remove after event processing

**Critical Rule**: Never `delete` an entity manually. Always use:
1. `entities_.Remove()` to extract ownership
2. `ProcessDeferredRemovals()` to destroy during event processing
3. `std::move()` for ownership transfers

---

## Workflow: Before Making Changes

**Step 1: Verify Existing Patterns**
1. Search codebase for related systems/usage
2. Check interface declarations for method signatures
3. Verify types (`pos_t` vs `int x, int y`)
4. Match established patterns
5. When uncertain, examine current usage first

**Step 2: Plan Integration**
1. Identify which systems the feature interacts with
2. Extend existing components/events if possible vs creating new ones
3. Ensure ownership/lifecycle patterns match existing code
4. Maintain separation of concerns (rendering, logic, state)

**Step 3: Implement Consistently**
- Use existing systems (events, factories, components) - don't create parallel systems
- Preserve type safety and RAII
- Keep architectural consistency
- Favor predictability over optimization

---

## Finding Existing Code

When implementing new features, search for existing patterns first:
```bash
# Find all event/action types
grep -r "class.*Action.*Event" include/
grep -r ": public Action" include/

# Find template usage
grep -r "TemplateRegistry::Instance().Create" src/

# Find component access patterns  
grep -r "GetComponent\|GetAttacker\|GetDestructible" src/

# Find entity lifecycle operations
grep -r "Spawn\|Remove\|entitiesToRemove_" src/

# Find Engine coordination methods
grep -r "Engine::.*Entity" include/Engine.hpp
```

**When adding a new feature, ask:**
1. Does a similar event/action already exist? (Check Event.hpp)
2. Can this be data-driven via JSON? (Check data/entities/)
3. Does this need a new component type? (Check Components.hpp)
4. How does this interact with entity lifecycle? (Check EntityManager usage)

---

## Anti-Patterns (Don't Do This)

### ❌ Special-Casing by Type
```cpp
// BAD: Type checking with branching
if (entity->GetFaction() == Faction::PLAYER) {
    // player-specific logic
} else if (entity->GetFaction() == Faction::MONSTER) {
    // monster-specific logic
}

// GOOD: Component-based polymorphism
if (auto* ai = entity->GetComponent<AiComponent>()) {
    ai->Act(engine);
}
```

### ❌ Separate X/Y Parameters
```cpp
// BAD
void SpawnEntity(int x, int y, const std::string& templateId);

// GOOD
void SpawnEntity(pos_t pos, const std::string& templateId);
```

### ❌ Manual Memory Management
```cpp
// BAD
Entity* entity = new Entity(...);
delete entity;

// GOOD
auto entity = std::make_unique<Entity>(...);
```

### ❌ Parallel Systems
```cpp
// BAD: Creating new event queue when one exists
std::vector<std::unique_ptr<CustomEvent>> myEvents;

// GOOD: Use Engine's event queue
engine.AddEventFront(event);
```

### ❌ Direct Component-to-Component Communication
```cpp
// BAD: Components calling each other
void DestructibleComponent::OnDeath() {
    entity_->GetRenderable()->ChangeIcon('%');  // Too tightly coupled
}

// GOOD: Entity coordinates component changes
void Entity::Die() {
    destructible_->MarkDead();
    renderable_->SetCorpseIcon();
    // Entity knows how to coordinate its components
}
```

### ❌ Immediate Entity Removal During Event Processing
```cpp
// BAD: Removes during iteration - iterator invalidation
void SomeAction::Execute() {
    entities_.Remove(&entity);  // CRASH: invalidates iterator in HandleEvents()
}

// GOOD: Deferred removal
void SomeAction::Execute() {
    engine_.HandleDeathEvent(entity);  // Marks for deferred removal
}
```

---

## Red Flags Requiring Refactoring
- Function needs many local variables → split it
- More than 3 indentation levels → extract helpers
- Repeated patterns → extract reusable functions
- `else` after `return` → remove redundant else
- Code needs comments to explain WHAT → refactor for clarity
- Special-case handling → seek uniform logic

---

## Quick Reference

### Creating an Entity from Template
```cpp
auto entity = TemplateRegistry::Instance().Create("orc", pos_t{10, 10});
entities_.Spawn(std::move(entity));
```

### Creating and Executing an Event
```cpp
auto action = std::make_unique<MoveAction>(engine, player, pos_t{0, -1});
engine.AddEventFront(action);
engine.HandleEvents();  // Processes all queued events
```

### Accessing Components
```cpp
// Safe access (returns nullptr if missing)
if (auto* attacker = entity->GetAttacker()) {
    int power = attacker->Attack();
}

// Required access (throws if missing)
auto& destructible = entity->RequireDestructible();
destructible.TakeDamage(damage);
```

### Working with Positions
```cpp
pos_t currentPos = entity->GetPos();
pos_t delta{0, -1};  // Move up
pos_t newPos = currentPos + delta;

if (engine.IsInBounds(newPos) && !engine.IsWall(newPos)) {
    entity->SetPos(newPos);
}
```

### Entity Lifecycle Operations
```cpp
// Spawning (EntityManager owns)
auto entity = TemplateRegistry::Instance().Create("orc", pos);
auto& ref = entities_.Spawn(std::move(entity));

// Deferred removal (safe during event processing)
entitiesToRemove_.push_back(&entity);  // Mark for removal
ProcessDeferredRemovals();              // Actually removes after turn

// Ownership transfer (e.g., pickup)
auto item = entities_.Remove(itemEntity);  // Extract from world
player->AddToInventory(std::move(item));   // Player now owns

// Dropping item back to world
auto item = player->ExtractFromInventory(itemIndex);  // Player releases
entities_.Spawn(std::move(item), playerPos);          // World owns again
```

### Engine Coordination Pattern
```cpp
// Let Engine coordinate cross-entity interactions
void Engine::DealDamage(Entity& target, int damage) {
    target.GetDestructible()->TakeDamage(damage);
    
    if (target.GetDestructible()->IsDead()) {
        HandleDeathEvent(target);  // Engine handles consequences
    }
}

// Don't coordinate directly in Actions - call Engine methods
void MeleeAction::Execute() {
    // ... calculate damage ...
    if (damage > 0) {
        engine_.DealDamage(*target, damage);  // Let Engine coordinate
    }
}
```

---

## Philosophy
This is production-quality code. Changes should enhance, not compromise, the architecture. Working code with clear intent beats theoretical perfection. When adding features, extend existing systems rather than creating new parallel ones. Consistency matters more than cleverness.

**Golden Rules:**
1. **Search first, implement second** - Similar patterns already exist
2. **Data in JSON, logic in C++** - Keep the separation clear
3. **Engine coordinates, components contain** - Respect the responsibility boundaries
4. **Defer deletions, transfer ownership explicitly** - Avoid dangling pointers
5. **Simple and consistent beats clever and unique** - Future maintainers will thank you
