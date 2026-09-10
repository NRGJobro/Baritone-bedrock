cmake_minimum_required(VERSION 4.0)

project(Limiter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(MSVC)
    add_compile_options(/MP)
endif()

option(BORION_BUILD_TESTS "Build pathfinder regression tests" OFF)
option(BORION_CORE_TESTS_ONLY "Build core tests without client dependencies" OFF)
if(BORION_BUILD_TESTS OR BORION_CORE_TESTS_ONLY)
    enable_testing()
    add_executable(BaritoneCoreTests tests/PathfinderTests.cpp
        src/Baritone/Core/Goal.cpp src/Baritone/Core/Movement.cpp
        src/Baritone/Core/Pathfinder.cpp)
    target_include_directories(BaritoneCoreTests PRIVATE src)
    if(MSVC)
        target_compile_options(BaritoneCoreTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(BaritoneCoreTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME BaritoneCoreTests COMMAND BaritoneCoreTests)
endif()
if(BORION_CORE_TESTS_ONLY)
    return()
endif()

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
