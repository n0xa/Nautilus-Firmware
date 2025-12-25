@echo off
REM Simple HTTP server launcher for the WebUSB flasher (Windows)
REM This script starts a local web server to serve the flasher interface

cd /d "%~dp0"

echo.
echo 🌊 Starting Nautilus WebUSB Flasher server...
echo.
echo Server will be available at:
echo   http://localhost:8000
echo.
echo Press Ctrl+C to stop the server
echo.

REM Try python first, then python3
where python >nul 2>nul
if %errorlevel% == 0 (
    python -m http.server 8000
    goto :end
)

where python3 >nul 2>nul
if %errorlevel% == 0 (
    python3 -m http.server 8000
    goto :end
)

where npx >nul 2>nul
if %errorlevel% == 0 (
    npx http-server -p 8000
    goto :end
)

echo Error: No suitable web server found!
echo Please install Python 3 or Node.js to run the server.
pause
exit /b 1

:end
