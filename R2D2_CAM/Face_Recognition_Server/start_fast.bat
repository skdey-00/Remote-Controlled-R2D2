@echo off
REM ============================================
REM R2D2 Face Recognition - FAST VERSION (OpenCV)
REM ============================================

title R2D2 Face Recognition - Fast Version
color 0A

echo.
echo ============================================
echo   R2D2 FACE RECOGNITION - FAST VERSION
echo ============================================
echo.
echo   Using OpenCV (fast processing + caching)
echo.
echo   First run: ~1-2 minutes (trains model)
echo   Future runs: ~1 second (loads saved model)
echo.
echo   Works with ESP32-CAM in this folder!
echo.
echo ============================================
echo.

REM Find Python command
set PYTHON_CMD=
py --version >nul 2>&1
if %errorlevel%==0 set PYTHON_CMD=py

if "%PYTHON_CMD%"=="" (
    python --version >nul 2>&1
    if %errorlevel%==0 set PYTHON_CMD=python
)

if "%PYTHON_CMD%"=="" (
    color 0C
    echo.
    echo ERROR: Python not found!
    echo.
    pause
    exit /b 1
)

echo [1/3] Python found:
%PYTHON_CMD% --version
echo.

REM Check dependencies
echo [2/3] Checking dependencies...
%PYTHON_CMD% -c "import cv2, flask, numpy, requests" >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo Installing required packages...
    %PYTHON_CMD% -m pip install opencv-python flask numpy requests
    if %errorlevel% neq 0 (
        echo ERROR: Failed to install dependencies
        pause
        exit /b 1
    )
)
echo Dependencies OK.
echo.

echo [3/3] Starting Face Recognition Server...
echo.
echo ============================================
echo   SERVER STARTING
echo ============================================
echo.
echo Make sure ESP32-CAM is:
echo   - Powered on
echo   - Running esp32cam_stream.ino from this folder
echo   - Connected to WiFi: R2D2_FaceRec (password: recognize)
echo.
echo Open browser: http://localhost:5000
echo.
echo Press Ctrl+C to stop
echo.
echo ============================================
echo.

%PYTHON_CMD% face_recognition_fast.py

echo.
echo Server stopped.
pause
