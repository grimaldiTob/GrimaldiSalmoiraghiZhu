#!/bin/bash

# Define paths to your executables
CPP_EXE="./build/astralog"
PYTHON_SCRIPT="-m src.astralog_collector --mode count --limit 12"

# function called on Ctrl+C
cleanup() {
    echo -e "\n[System] Stopping all processes..."
    kill "$CPP_PID" 2>/dev/null
    kill "$PY_PID" 2>/dev/null
    exit 0
}

# Trap Ctrl+C (SIGINT) and SIGTERM to trigger cleanup
trap cleanup SIGINT SIGTERM

echo "Starting Python data collector..."
python3 $PYTHON_SCRIPT &
PY_PID=$!

echo "Starting the System..."
$CPP_EXE &
CPP_PID=$!

echo "Application running. Press [CTRL+C] to stop."

wait $CPP_PID $PY_PID