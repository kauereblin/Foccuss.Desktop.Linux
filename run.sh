#!/bin/bash
echo "Running Foccuss..."

if [ ! -f "build/Foccuss" ]; then
    echo "Executable not found, building first..."
    ./build.sh
    if [ $? -ne 0 ]; then
        echo "Build failed!"
        exit $?
    fi
fi

echo "Starting Foccuss application..."
./build/Foccuss 