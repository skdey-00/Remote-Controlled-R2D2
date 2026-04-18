@echo off
echo ========================================
echo R2D2 Face Recognition Server
echo ========================================
echo.

REM Check if Python is installed
py --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.8+ from https://www.python.org/
    pause
    exit /b 1
)

echo Checking dependencies...
py -c "import face_recognition" >nul 2>&1
if errorlevel 1 (
    echo Installing required packages...
    py -m pip install dlib-bin opencv-python face_recognition Pillow Flask
    if errorlevel 1 (
        echo.
        echo ERROR: Failed to install dependencies
        pause
        exit /b 1
    )
)

echo.
echo All dependencies installed!
echo.
echo ========================================
echo SETUP INSTRUCTIONS:
echo ========================================
echo.
echo 1. Upload esp32cam_stream.ino to ESP32-CAM
echo 2. Connect laptop to WiFi: "R2D2_FaceRec" (password: recognize)
echo 3. Open browser to: http://localhost:5000
echo.
echo Starting Face Recognition Server...
echo Press Ctrl+C to stop
echo ========================================
echo.

py face_recognition_server.py

pause
