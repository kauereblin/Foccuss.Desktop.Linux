#!/bin/bash

echo "Fixing build issues and building Foccuss..."

# Stop any running instances of Foccuss
echo "Stopping any running instances of Foccuss..."
pkill -f Foccuss || true
sleep 2  # Wait for the process to fully terminate

# Clean build directory
echo "Cleaning build directory..."
if [ -d "build" ]; then
    rm -rf build
fi
mkdir build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -G "Unix Makefiles"
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    cd ..
    exit 1
fi

# Build the project
echo "Building with CMake..."
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

# Copy additional libraries if needed
echo "Copying required libraries..."
mkdir -p Release/platforms
mkdir -p Release/styles
mkdir -p Release/sqldrivers

echo "Build successful!"
echo "Executable location: $(pwd)/Release/Foccuss"
cd .. 