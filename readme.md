# Limiter

A Minecraft Bedrock port of Baritone built on the Borion/MCRenderTests SDK surface. The old Borion combat, exploit, movement, and utility feature set has been removed so the client can focus on autonomous navigation and world processes. The only ClickGUI card is Limiter.

## Controls

- `Tab` opens the MCRenderTests ClickGUI.
- `End` unloads the client.
- `.goto x y z` sets a block goal and starts pathing.
- `.goal x y z` sets a goal without starting.
- `.path`, `.stop`, `.pause`, `.resume`, and `.status` control execution.
- `.xz x z`, `.y level`, and `.near x y z radius` select other goal types.
- `.water on/off` and `.fall 0-64` change core traversal policy.
- `.axis`, `.thisway distance`, `.away x y z distance`, and `.surface` expose additional Baritone goal types.
- `.mine diamond_ore 8 32` scans loaded blocks for both regular and deepslate diamond ore, plans a safe two-block-high tunnel through breakable walls while avoiding water and damage blocks, equips the best hotbar tool, and mines up to eight matches in a 32-block radius. Numeric runtime IDs and comma-separated filters are also accepted; a count of `0` mines continuously.
- `.tunnel` clears a 1x2x32 passage in the facing direction; `.tunnel height width depth` supports bounded custom dimensions.
- `.explore [chunkRadius]` visits chunk centers in expanding rings; radius `0` explores continuously.
- `.wp save/goto/delete name` and `.wp list` manage session waypoints.
- `.eta` estimates remaining path ticks and `.set` exposes advanced route budgets and movement policies.
- `.b ...` remains available as a namespaced alternative.

The ClickGUI includes Limiter and FullBright module cards. Limiter exposes movement toggles for diagonal traversal, water, one-block ascents, drops, sprinting, and stuck replanning. Visual toggles control the current path, animated goal, calculation details, fading, and line/ribbon geometry; active mining targets use cyan one-block markers visible through walls. FullBright forces both Bedrock's gamma lookup and player-vision render pass to maximum brightness while enabled, with vanilla lighting restored when disabled.

## Current port scope

The C++ core includes Baritone-style goals (block, XZ, Y level, near, composite, interaction, two/three-block, axis, and run-away), incremental A*, movement-cost/ETA tracking, partial paths at unloaded chunk boundaries, cardinal/diagonal traversal, one-block ascent, descent/fall, water traversal, hazard avoidance, break-aware mining routes, parkour, bridge construction, cancellation, pause/resume, stuck detection, and replanning. The Bedrock adapter reads blocks through `BlockSource`, drives the local player's ECS movement components, performs ordinary `GameMode` breaking/placement, and renders the active path.

The first process layer now ports mining, loaded-world exploration, and session waypoints. Schematic building, farming, entity following, persistent chunk caching, and automatic tool/inventory management remain later layers.

## Build

Configure with CMake 4.x and MSVC for x64, then build the `Borion` target; the output is `Limiter.dll`. `BaritoneCoreTests` tests the platform-neutral pathfinder without Minecraft.

## Licensing

This project is a C++ adaptation informed by Baritone. Baritone is LGPL-3.0; its license is included as `LICENSE-Baritone`. Keep the corresponding source and notices available when distributing builds.
