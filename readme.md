# Limiter

Limiter is a focused Minecraft Bedrock navigation client. It provides pathfinding,
mining, exploration, waypoints, Elytra travel, and a small set of supporting
modules without carrying a general-purpose client feature set.

## Modules

- **Limiter** — navigation, mining, exploration, and route rendering
- **Camera Tweaks** — configurable third-person camera controls
- **FullBright** — maximum world brightness while enabled
- **GUI Move** — normal movement while supported game screens are open

Press `Tab` to open the ClickGUI and `End` to unload the client.

## Commands

Commands can be entered directly, with `.l`, or with the compatible `.b`
prefix. Run `.help` in game for the complete list.

Common commands:

- `.goto x y z`, `.goal x y z`, `.path`, and `.stop`
- `.pause`, `.resume`, `.status`, and `.eta`
- `.mine diamond_ore 8 32`
- `.tunnel [height width depth]`
- `.explore [chunkRadius]`
- `.wp save/goto/delete name` and `.wp list`
- `.water on/off`, `.fall 0-64`, and `.set`

## Source layout

- `src/Baritone` contains the licensed navigation engine and Bedrock adapter.
- `src/Client` contains Limiter's UI and modules.
- `src/Memory` contains signatures and hooks.
- `src/SDK` contains the minimal Minecraft types required by the client.
- `src/Utils` contains shared rendering, logging, and platform helpers.
- `tests` contains platform-neutral pathfinding regression tests.

The client source list is explicit in `CMakeLists.txt`; adding a file to `src`
does not silently add it to the DLL.

## Building

Limiter requires CMake 3.28 or newer and Clang-CL with the Visual Studio x64
toolchain. From a Visual Studio developer prompt:

```bat
cmake --preset x64-release
cmake --build --preset x64-release
ctest --preset x64-release
```

The client is written to `out/build/x64-release/Limiter.dll`.

## Licensing

The navigation engine is a C++ adaptation informed by Baritone. Baritone is
licensed under LGPL-3.0; its license is included as `LICENSE-Baritone`.
Retain the license and corresponding notices when distributing Limiter.
