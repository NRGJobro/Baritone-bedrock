# Borion Baritone

A bare-bones Minecraft Bedrock pathfinding client built on the Borion/MCRenderTests SDK surface. The old Borion combat, exploit, movement, and utility feature set has been removed. The only ClickGUI card is Baritone.

## Controls

- `Tab` opens the MCRenderTests ClickGUI.
- `End` unloads the client.
- `.goto x y z` sets a block goal and starts pathing.
- `.goal x y z` sets a goal without starting.
- `.path`, `.stop`, `.pause`, `.resume`, and `.status` control execution.
- `.xz x z`, `.y level`, and `.near x y z radius` select other goal types.
- `.water on/off` and `.fall 0-20` change core traversal policy.
- `.b ...` remains available as a namespaced alternative.

The ClickGUI exposes movement toggles for diagonal traversal, water, one-block ascents, drops, sprinting, and stuck replanning. Visual toggles control the current path, animated goal, calculation details, fading, and line/ribbon geometry.

## Current port scope

The C++ core includes Baritone-style goals, incremental A*, partial paths at unloaded chunk boundaries, cardinal/diagonal traversal, one-block ascent, descent/fall, water traversal, hazard avoidance, cancellation, pause/resume, stuck detection, and replanning. The Bedrock adapter reads blocks through `BlockSource`, drives the local player's ECS movement components, and renders the active path.

Higher-level Java Baritone processes such as mining, building schematics, farming, exploration, inventory/tool selection, block breaking, and block placement are intentionally left as later layers over this core.

## Build

Configure with CMake 4.x and MSVC for x64, then build the `Borion` target. `BaritoneCoreTests` is enabled by default and tests the platform-neutral pathfinder without Minecraft.

## Licensing

This project is a C++ adaptation informed by Baritone. Baritone is LGPL-3.0; its license is included as `LICENSE-Baritone`. Keep the corresponding source and notices available when distributing builds.
