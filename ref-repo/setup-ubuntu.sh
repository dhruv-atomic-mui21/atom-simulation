#!/bin/bash
set -e

echo "Installing native C++ dependencies..."
sudo apt update
sudo apt install -y build-essential cmake libglew-dev libglfw3-dev libglm-dev libgl1-mesa-dev

echo "Configuring project with CMake..."
cmake -B build -S .

echo "Building the project..."
cmake --build build

echo "Build complete! You can run the program from the build/ directory (e.g., ./build/atom)"