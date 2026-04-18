@echo off
REM ============================================
REM Install Dependencies (Run Once)
REM Run this while connected to HOME WiFi
REM ============================================

echo ============================================
echo   R2D2 - Install Dependencies
echo ============================================
echo.
echo STEP 1: Connect to your HOME WiFi first
echo STEP 2: Then run this script
echo.
pause

REM Check Python
py --version >nul 2>&1
if %errorlevel% neq 0 (
    python --version >nul 2>&1
    if %errorlevel% neq 0 (
        echo ERROR: Python not found!
        pause
        exit /b 1
    )
    set PYTHON_CMD=python
) else (
    set PYTHON_CMD=py
)

echo Using: %PYTHON_CMD%
echo.

REM Create virtual environment
if not exist "venv\" (
    echo Creating virtual environment...
    %PYTHON_CMD% -m venv venv
)

echo Activating virtual environment...
call venv\Scripts\activate.bat

echo.
echo Installing dependencies...
echo This will take several minutes...
echo.

%PYTHON_CMD% -m pip install --upgrade pip
%PYTHON_CMD% -m pip install flask opencv-contrib-python numpy requests

echo.
echo ============================================
echo   INSTALLATION COMPLETE!
echo ============================================
echo.
echo Now you can:
echo 1. Connect to R2D2-CAM WiFi
echo 2. Run start_face_recognition.bat
echo.
pause
