#!/bin/bash

echo "========================================"
echo "R2D2 Face Recognition Server"
echo "========================================"
echo ""

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "ERROR: Python 3 is not installed"
    echo "Please install Python 3.8+ from https://www.python.org/"
    exit 1
fi

echo "Checking dependencies..."
if ! python3 -c "import face_recognition" 2>/dev/null; then
    echo "Installing required packages..."
    pip3 install -r requirements.txt
    if [ $? -ne 0 ]; then
        echo ""
        echo "ERROR: Failed to install dependencies"
        echo "Make sure you have pip installed and internet connection"
        exit 1
    fi
fi

echo ""
echo "All dependencies installed!"
echo ""
echo "Starting Face Recognition Server..."
echo ""
echo "Open your browser to: http://localhost:5000"
echo ""
echo "Press Ctrl+C to stop the server"
echo "========================================"
echo ""

python3 face_recognition_server.py
