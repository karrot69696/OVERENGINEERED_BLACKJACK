cmake_minimum_required(VERSION 3.20)

# Project definition
project(MindJack
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

# set SFML Cmake directory
set(SFML_DIR "C:/Users/minhk/OneDrive/Desktop/Downloads/SFML-3.0.2/lib/cmake/SFML")
# find the SFML require packages
find_package(SFML 3 COMPONENTS System Window Graphics Audio Network REQUIRED)



# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-I "C:/Users/minhk/OneDrive/Desktop/Downloads/SFML-3.0.2/include")
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
    src/skillEngine/Skill_NeuralGambit.h
)

set(GAME_ENGINE_SOURCES
    src/gameEngine/Game.cpp
    src/gameEngine/UIManager.cpp
    src/gameEngine/RoundManager.cpp
    src/gameEngine/AnimationManager.cpp
)

set(GAME_ENGINE_HEADERS
    src/gameEngine/Game.h
    src/gameEngine/UIManager.h
    src/gameEngine/RoundManager.h
    src/gameEngine/AnimationManager.h
)

set(PHASE_ENGINE_SOURCES
    src/phaseEngine/Phase.cpp
    src/phaseEngine/PlayerHitPhase.cpp
    src/phaseEngine/HostHitPhase.cpp
    src/phaseEngine/BattlePhase.cpp
    src/phaseEngine/BlackJackCheckPhase.cpp
    src/phaseEngine/RoundEndPhase.cpp
)

set(PHASE_ENGINE_HEADERS
    src/phaseEngine/Phase.h
    src/phaseEngine/PlayerHitPhase.h
    src/phaseEngine/HostHitPhase.h
    src/phaseEngine/BattlePhase.h
    src/phaseEngine/BlackJackCheckPhase.h
    src/phaseEngine/RoundEndPhase.h
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

# Create static library for phase engine
add_library(PhaseEngine STATIC
    ${PHASE_ENGINE_SOURCES}
    ${PHASE_ENGINE_HEADERS}
)

target_include_directories(PhaseEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhaseEngine
    PUBLIC
        LowLevelEntities
        SkillEngine
)

# Main executable (assumes there's a main.cpp somewhere)
# Uncomment and adjust when you have your main entry point
add_executable(MindJack
    src/main.cpp
)

# copy src/assets to build directory
add_custom_command(TARGET MindJack POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "C:/Users/minhk/OneDrive/Desktop/Downloads/SFML-3.0.2/bin"
    $<TARGET_FILE_DIR:MindJack>
)

target_link_libraries(MindJack
    PRIVATE
        GameEngine
        SkillEngine
        LowLevelEntities
        PhaseEngine
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
        SFML::Network
)
# copy SFML dlls to build directory after build
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/assets
    ${CMAKE_BINARY_DIR}/assets
)
# Installation rules
install(TARGETS LowLevelEntities SkillEngine GameEngine PhaseEngine
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