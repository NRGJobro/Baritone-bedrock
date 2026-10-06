cmake_minimum_required(VERSION 3.28)

project(Limiter LANGUAGES CXX)

# Minecraft 1.26's EnTT component pool identifiers must be generated with the
# same Clang-CL ABI used by Phase. MSVC produces different type identities and
# silently makes Actor::tryGet return null for every runtime component.
if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Limiter requires Clang-CL for the current Minecraft EnTT ABI. Use the supplied presets.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(LIMITER_BUILD_TESTS "Build pathfinder regression tests" OFF)
option(LIMITER_CORE_TESTS_ONLY "Build core tests without client dependencies" OFF)
option(LIMITER_LINK_MAP "Generate a linker map for crash-address analysis" OFF)
option(LIMITER_UNITY_BUILD "Batch source files into unity translation units" ON)

if(LIMITER_BUILD_TESTS OR LIMITER_CORE_TESTS_ONLY)
    enable_testing()
    add_executable(LimiterCoreTests tests/PathfinderTests.cpp
        src/Baritone/Core/Goal.cpp src/Baritone/Core/Movement.cpp
        src/Baritone/Core/Pathfinder.cpp)
    target_compile_definitions(LimiterCoreTests PRIVATE _USE_STD_VECTOR_ALGORITHMS=0)
    target_include_directories(LimiterCoreTests PRIVATE src)
    if(MSVC)
        target_compile_options(LimiterCoreTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(LimiterCoreTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME LimiterCoreTests COMMAND LimiterCoreTests)
endif()
if(LIMITER_CORE_TESTS_ONLY)
    return()
endif()

include(include/FetchLibraries.cmake)

set(LIMITER_SOURCES
    src/main.cpp
    src/Client.cpp
    src/Client/GUI/ClickGui.cpp
    src/Client/Modules/Module.cpp
    src/Client/Modules/ModuleManager.cpp
    src/Client/Modules/LimiterModule.cpp
    src/Client/Modules/CameraTweaksModule.cpp
    src/Client/Modules/FullBrightModule.cpp
    src/Client/Modules/GuiMoveModule.cpp
    src/Memory/Hook/Hook.cpp
    src/Memory/Hook/HookManager.cpp
    src/Memory/Hook/Hooks/ClientHooks.cpp
    src/Memory/Hook/Hooks/RenderHooks.cpp
    src/Memory/Sig/SigInit.cpp
    src/Memory/Sig/Signature.cpp
    src/Memory/Sig/SignatureManager.cpp
    src/Baritone/BaritoneController.cpp
    src/Baritone/Bedrock/BedrockBlockBreaking.cpp
    src/Baritone/Bedrock/BedrockWorld.cpp
    src/Baritone/Bedrock/ElytraProcess.cpp
    src/Baritone/Bedrock/ExploreProcess.cpp
    src/Baritone/Bedrock/MiningProcess.cpp
    src/Baritone/Bedrock/PathExecutor.cpp
    src/Baritone/Bedrock/PathRenderer.cpp
    src/Baritone/Core/Goal.cpp
    src/Baritone/Core/Movement.cpp
    src/Baritone/Core/Pathfinder.cpp
    src/SDK/MC.cpp
    src/SDK/Client/ClientInstance.cpp
    src/SDK/Client/MinecraftGame.cpp
    src/SDK/Client/Font/Font.cpp
    src/SDK/Client/GUI/GuiData.cpp
    src/SDK/Client/MCE/Color.cpp
    src/SDK/Client/MCE/MaterialPtr.cpp
    src/SDK/Client/MCE/Mesh.cpp
    src/SDK/Client/MCE/MeshContext.cpp
    src/SDK/Client/MCE/MeshData.cpp
    src/SDK/Network/Packet/Packet.cpp
    src/SDK/Render/Level/LevelRenderer.cpp
    src/SDK/Render/Level/LevelRendererPlayer.cpp
    src/SDK/Render/MeshHelpers.cpp
    src/SDK/Render/MinecraftUIRenderContext.cpp
    src/SDK/Render/ScreenContext.cpp
    src/SDK/Render/Tessellator.cpp
    src/SDK/Screen/ScreenView.cpp
    src/SDK/Screen/UIControl.cpp
    src/SDK/Screen/VisualTree.cpp
    src/SDK/World/Actor/Actor.cpp
    src/SDK/World/Actor/Player.cpp
    src/SDK/World/Block/Block.cpp
    src/SDK/World/Block/BlockLegacy.cpp
    src/SDK/World/BlockSource.cpp
    src/SDK/World/Inventory/Inventory.cpp
    src/SDK/World/Inventory/PlayerInventory.cpp
    src/SDK/World/Item/ItemStackBase.cpp
    src/SDK/World/Level/Level.cpp
    src/Utils/DrawUtils.cpp
    src/Utils/LimiterTess.cpp
    src/Utils/Logger.cpp
    src/Utils/TimeUtils.cpp
    src/Utils/Utils.cpp
)

add_library(Limiter SHARED ${LIMITER_SOURCES})
target_compile_definitions(Limiter PRIVATE NOMINMAX _USE_STD_VECTOR_ALGORITHMS=0)
target_link_libraries(Limiter PRIVATE minhook libhat EnTT::EnTT magic_enum spdlog::spdlog glm)
target_precompile_headers(Limiter PRIVATE PCH.h)
set_target_properties(Limiter PROPERTIES
    UNITY_BUILD ${LIMITER_UNITY_BUILD}
    UNITY_BUILD_BATCH_SIZE 8
)
set_source_files_properties(
    src/Baritone/Bedrock/MiningProcess.cpp
    src/Baritone/Bedrock/PathExecutor.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

if(MSVC AND LIMITER_LINK_MAP)
    target_link_options(Limiter PRIVATE /MAP:${CMAKE_BINARY_DIR}/Limiter.map /MAPINFO:EXPORTS)
endif()

set_property(
    TARGET Limiter libhat minhook spdlog glm
    PROPERTY MSVC_RUNTIME_LIBRARY
    $<$<CONFIG:Debug>:MultiThreadedDLL>$<$<CONFIG:Release>:MultiThreaded>
)
