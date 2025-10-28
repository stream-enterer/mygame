-  Mouselook info dialog should also show when you walk over items
    - However, mouselook will always have priority (if the mouse is moved, otherwise ignore it. I may not be explaining the intended behavior properly, you can try and figure out what i mean based on common patterns in game design)

-  Stack dropped/carried items (e.g. 1x 2x Health Potion)
-  diagnonal movement
-  autopickup
-  Click to move
-  Numpad movement
-  Rebindable hotkeys
-  autoexplore
-  number munchers
-  samurai kirby (arcade reflexes, smoother graphics)
-  tileset
-  sound
-  Exile-like spell effects/targeting lines
-  Height
-  Realmz-like targeting system
-  Add dice style monster spawn groups (AD&D 2.5 style)
-  JSON lists (JSON levels can include batch spawn lists)
-  Vaults
- Make level-loading order data-driven
- Tab targetting
- Multidrop
- Wayland
- Remember items "poorly" that are in fog
- Reticle target automatically begins on nearest monster
- Tab targetting
- Nitpick: Health bar should be called status bar
- Feature: Scrolling menus
- Feature: Clicking in menus/level up menu
- Bug?: Targetting scroll on enemy, then using it, then enemy dies, reveals mouselook name on top left of corpse. not sure


- Feature: Staircases, once discovered, should remain visible within the fog of war when when the player moves out of FOV. They should still have the FOV background color around them. Make this generally applicable to other things so it's not a special case.
- Feature: Targeting on self also closes targeting. I want targeting on playerto ask "Are you sure you want to cast X?" and to work without prompting if it's a buff spell.
Bug: You can level up while dead if you're getting attacked. THe level up screen appears while your character is dead underneath. Not sure how this happened. I was getting attacked by several monsters and holding down the attack on them and then leveled up and died immediately. I assume level up isn't taking a turn but I dunno. Maybe it's an ordering issue with the other monsters attacking after I killed one monster and dealing lethal damage to me while a level up was on the queue, or something like that.
- Enhancement: Consolidate Power/STR and Defense/DEX as regards Getter for defense nad player Json.
Bug: Pressing enter after death starts a new game without bringing up character creation (change behavior or remove this all together)

- Good! I can see that name_ is a protected member, meaning derived classes could potentially modify it, but there's no public SetName() method. For now, let's keep the factory-based approach and note that we're sacrificing custom corpse names for architectural consistency. This is actually acceptable - all corpses will just be called "corpse" which is fine.
- Low Priority #9: Inventory component (premature optimization)
- Low Priority #10: Config file for visual constants (rarely changed values)

- Enhancement: Move strings for items and dungeon levels from locale file to their individual jsons


- Bug: Confirm menu needs to give an error message if you haven't selected a race or class when you press enter
- Bug: When you select a scroll to target, then esc out of it or press enter on yourself and the targeting cancels, the mouselook thing in the top left still says "player"
