#!/bin/bash

# Script to run Foccuss service in the foreground for testing and debugging
# Used for development purposes

# Echo with color for better visibility
echo_green() {
    echo -e "\e[32m$1\e[0m"
}

echo_red() {
    echo -e "\e[31m$1\e[0m"
}

echo_yellow() {
    echo -e "\e[33m$1\e[0m"
}

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Find the Foccuss binary
if [ -f "$SCRIPT_DIR/Foccuss" ]; then
    FOCCUSS_BIN="$SCRIPT_DIR/Foccuss"
elif [ -f "$SCRIPT_DIR/build/Foccuss" ]; then
    FOCCUSS_BIN="$SCRIPT_DIR/build/Foccuss"
else
    # Try to find it in subdirectories
    FOCCUSS_BIN=$(find "$SCRIPT_DIR" -name "Foccuss" -type f -executable | head -1)
    
    if [ -z "$FOCCUSS_BIN" ]; then
        echo_red "Error: Could not find Foccuss executable. Please specify the path."
        echo_red "Usage: $0 [path/to/Foccuss]"
        exit 1
    fi
fi

# If user provided a specific path to the binary
if [ -n "$1" ] && [ -f "$1" ]; then
    FOCCUSS_BIN="$1"
fi

echo_yellow "Checking if Foccuss service is already running..."

# Check for existing service processes
FOCCUSS_PIDS=$(pgrep -f "Foccuss.*--service" 2>/dev/null)
if [ -n "$FOCCUSS_PIDS" ]; then
    echo_red "Foccuss service is already running with PIDs: $FOCCUSS_PIDS"
    echo_red "Please stop existing services before starting a new one."
    echo_red "You can use: ./kill_foccuss.sh"
    exit 1
fi

echo_green "Starting Foccuss in service mode for testing..."
echo_yellow "Using binary: $FOCCUSS_BIN"
echo_yellow "Press Ctrl+C to stop the service"
echo ""

# Run Foccuss in the foreground with service flag
"$FOCCUSS_BIN" --service 