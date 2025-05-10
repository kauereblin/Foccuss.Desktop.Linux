#!/bin/bash

# Script to terminate Foccuss background processes
# Used for development and testing purposes

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

# Check if we should kill GUI processes too
KILL_ALL=false
if [ "$1" == "--kill-all" ]; then
    KILL_ALL=true
    echo_yellow "WARNING: Will terminate ALL Foccuss processes including GUI instances."
fi

echo_yellow "Stopping Foccuss service..."

# Stop Foccuss service via systemctl if it's running
if systemctl --user is-active foccuss.service &>/dev/null; then
    systemctl --user stop foccuss.service
    echo_green "Foccuss systemd service stopped."
else
    echo_yellow "Foccuss systemd service not running."
fi

# Find and kill any remaining Foccuss processes that might be running
FOCCUSS_PIDS=$(pgrep -f "Foccuss.*--service" 2>/dev/null)

if [ -n "$FOCCUSS_PIDS" ]; then
    echo_yellow "Found Foccuss processes running with PIDs: $FOCCUSS_PIDS"
    
    for PID in $FOCCUSS_PIDS; do
        echo_yellow "Killing Foccuss process with PID: $PID"
        kill -15 $PID 2>/dev/null
    done
    
    # Check if they actually died
    sleep 1
    REMAINING=$(pgrep -f "Foccuss.*--service" 2>/dev/null)
    
    if [ -n "$REMAINING" ]; then
        echo_red "Some processes still running. Forcing termination..."
        for PID in $REMAINING; do
            echo_red "Force killing PID: $PID"
            kill -9 $PID 2>/dev/null
        done
    fi
    
    echo_green "All Foccuss service processes terminated."
else
    echo_yellow "No Foccuss service processes found running."
fi

# Check for GUI instances
GUI_PIDS=$(pgrep -f "Foccuss" | grep -v "Foccuss.*--service" 2>/dev/null)

if [ -n "$GUI_PIDS" ]; then
    echo_yellow "Found Foccuss GUI processes running with PIDs: $GUI_PIDS"
    
    if [ "$KILL_ALL" = true ]; then
        for PID in $GUI_PIDS; do
            echo_yellow "Killing Foccuss GUI process with PID: $PID"
            kill -15 $PID 2>/dev/null
        done
        
        # Check if they died
        sleep 1
        REMAINING_GUI=$(pgrep -f "Foccuss" | grep -v "Foccuss.*--service" 2>/dev/null)
        
        if [ -n "$REMAINING_GUI" ]; then
            echo_red "Some GUI processes still running. Forcing termination..."
            for PID in $REMAINING_GUI; do
                echo_red "Force killing GUI PID: $PID"
                kill -9 $PID 2>/dev/null
            done
        fi
        
        echo_green "All Foccuss GUI processes terminated."
    else
        echo_yellow "NOTE: GUI processes were not terminated. Run with --kill-all to terminate GUI processes as well."
    fi
else
    echo_yellow "No Foccuss GUI processes found running."
fi

echo_green "Done. You can now edit and test your application." 