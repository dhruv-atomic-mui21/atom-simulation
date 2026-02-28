cmake_minimum_required(VERSION 3.16)
project(ElementSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ──────────────────────────────────────────────
find_package(OpenGL REQUIRED)
find_package(GLEW   REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm    CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ── Source files ──────────────────────────────────────────────
set(SOURCES
    src/main.cpp

    # Engine
    src/engine/engine.cpp
    src/engine/camera.cpp
    src/engine/renderer.cpp

    # Physics
    src/physics/element.cpp
    src/physics/atom.cpp
    src/physics/electron.cpp
    src/physics/quantum.cpp
    src/physics/interaction.cpp
    src/physics/simulation.cpp

    # UI
    src/ui/periodic_table.cpp
    src/ui/hud.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
)

# ── Copy data directory next to executable ────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
)
