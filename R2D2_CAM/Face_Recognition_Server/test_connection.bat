@echo off
echo Running ESP32 Connection Test...
echo.

REM Find Python
py --version >nul 2>&1
if %errorlevel%==0 (
    py test_connection.py
    pause
    exit /b 0
)

python --version >nul 2>&1
if %errorlevel%==0 (
    python test_connection.py
    pause
    exit /b 0
)

echo ERROR: Python not found!
pause
