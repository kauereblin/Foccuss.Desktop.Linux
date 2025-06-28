#!/bin/bash
echo "Building Foccuss..."

echo "Creating icons..."
./create_icons.sh

if [ ! -d "build" ]; then
    mkdir build
fi
cd build

echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit $?
fi

echo "Building with CMake..."
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit $?
fi

echo "Build successful!"
echo "Executable location: $(pwd)/Foccuss"

cd .. 