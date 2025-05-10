#!/bin/bash

# Exit on error
set -e

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Run CMake if CMakeCache.txt doesn't exist
if [ ! -f "CMakeCache.txt" ]; then
    cmake ..
fi

# Build the project
make -j$(nproc)

echo "Build completed successfully!"
echo "Running Foccuss..."

# Run the application
./Foccuss 