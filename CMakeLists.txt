cmake_minimum_required(VERSION 4.0)

project(BorionBaritone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_compile_options(/MP)

include(include/FetchLibraries.cmake)

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

add_library(Borion SHARED ${CLIENT_SOURCES})
target_link_libraries(Borion PRIVATE minhook libhat EnTT::EnTT magic_enum spdlog::spdlog glm dwmapi.lib)
target_precompile_headers(Borion PRIVATE "PCH.cpp")

set_property(
    TARGET Borion libhat minhook spdlog glm
    PROPERTY MSVC_RUNTIME_LIBRARY
    $<$<CONFIG:Debug>:MultiThreadedDLL>$<$<CONFIG:Release>:MultiThreaded>
)

option(BORION_BUILD_TESTS "Build platform-independent Baritone core tests" ON)

if(BORION_BUILD_TESTS)
    enable_testing()
    add_executable(BaritoneCoreTests
        tests/PathfinderTests.cpp
        src/Baritone/Core/Goal.cpp
        src/Baritone/Core/Movement.cpp
        src/Baritone/Core/Pathfinder.cpp
    )
    target_include_directories(BaritoneCoreTests PRIVATE src)
    target_link_libraries(BaritoneCoreTests PRIVATE glm)
    add_test(NAME BaritoneCoreTests COMMAND BaritoneCoreTests)
endif()
