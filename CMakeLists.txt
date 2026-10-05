cmake_minimum_required(VERSION 3.28)

project(Limiter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(LIMITER_BUILD_TESTS "Build pathfinder regression tests" OFF)
option(LIMITER_CORE_TESTS_ONLY "Build core tests without client dependencies" OFF)
option(LIMITER_LINK_MAP "Generate a linker map for crash-address analysis" OFF)

if(LIMITER_BUILD_TESTS OR LIMITER_CORE_TESTS_ONLY)
    enable_testing()
    add_executable(LimiterCoreTests tests/PathfinderTests.cpp
        src/Baritone/Core/Goal.cpp src/Baritone/Core/Movement.cpp
        src/Baritone/Core/Pathfinder.cpp)
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

file(GLOB_RECURSE LIMITER_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(Limiter SHARED ${LIMITER_SOURCES})
target_compile_definitions(Limiter PRIVATE NOMINMAX)
target_link_libraries(Limiter PRIVATE minhook libhat EnTT::EnTT magic_enum spdlog::spdlog glm dwmapi.lib)
target_precompile_headers(Limiter PRIVATE PCH.h)
set_target_properties(Limiter PROPERTIES
    UNITY_BUILD ON
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
