cmake_minimum_required(VERSION 3.10)
project(AtomSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find required dependencies
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# 2D Bohr Model executable
add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE OpenGL::GL GLEW::GLEW glfw glm::glm)