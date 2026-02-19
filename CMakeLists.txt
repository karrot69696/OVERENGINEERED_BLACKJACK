cmake_minimum_required(VERSION 3.20)

# Project definition
project(OverengineeredBlackjack
    VERSION 1.0.0
    DESCRIPTION "An overengineered blackjack game with skill system"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files organization
set(LOW_LEVEL_SOURCES
    src/lowLevelEntities/Card.cpp
    src/lowLevelEntities/Deck.cpp
    src/lowLevelEntities/GameState.cpp
    src/lowLevelEntities/Player.cpp
)

set(LOW_LEVEL_HEADERS
    src/lowLevelEntities/Card.h
    src/lowLevelEntities/Deck.h
    src/lowLevelEntities/GameState.h
    src/lowLevelEntities/Player.h
    src/lowLevelEntities/SkillDeck.h
)

set(SKILL_ENGINE_SOURCES
    src/skillEngine/skillManager.cpp
)

set(SKILL_ENGINE_HEADERS
    src/skillEngine/Skill.h
    src/skillEngine/SkillManager.h
    src/skillEngine/Skill_Deliverance.h
)

set(GAME_ENGINE_SOURCES
    src/gameEngine/Game.cpp
)

set(GAME_ENGINE_HEADERS
    src/gameEngine/Game.h
)

# Create static library for low level entities
add_library(LowLevelEntities STATIC
    ${LOW_LEVEL_SOURCES}
    ${LOW_LEVEL_HEADERS}
)

target_include_directories(LowLevelEntities
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Create static library for skill engine
add_library(SkillEngine STATIC
    ${SKILL_ENGINE_SOURCES}
    ${SKILL_ENGINE_HEADERS}
)

target_include_directories(SkillEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SkillEngine
    PUBLIC
        LowLevelEntities
)

# Create static library for game engine
add_library(GameEngine STATIC
    ${GAME_ENGINE_SOURCES}
    ${GAME_ENGINE_HEADERS}
)

target_include_directories(GameEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GameEngine
    PUBLIC
        LowLevelEntities
        SkillEngine
)

# Main executable (assumes there's a main.cpp somewhere)
# Uncomment and adjust when you have your main entry point
add_executable(blackjack
    src/main.cpp
)

target_link_libraries(blackjack
    PRIVATE
        GameEngine
        SkillEngine
        LowLevelEntities
)

# Installation rules
install(TARGETS LowLevelEntities SkillEngine GameEngine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Optional: Enable testing
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    # add_subdirectory(tests)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")