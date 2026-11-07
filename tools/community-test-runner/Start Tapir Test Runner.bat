@echo off
rem Double-click this file to start the Tapir test runner.
rem The first time, it asks a few questions (setup); after that it opens the runner's window.
setlocal
cd /d "%~dp0"
title Tapir test runner

set "PY="
set "PYW="
py -3 -c "import sys, tkinter; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 (
    set "PY=py -3"
    set "PYW=pyw -3"
    goto found
)
python -c "import sys, tkinter; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 (
    set "PY=python"
    set "PYW=pythonw"
    goto found
)

echo.
echo  Python 3.10 or newer is needed and was not found.
echo.
echo  The download page opens now. Download the latest Python for Windows,
echo  run it, tick "Add python.exe to PATH" on the first screen, and click
echo  "Install Now". Then double-click this file again.
echo.
start "" "https://www.python.org/downloads/windows/"
pause
exit /b 1

:found
if not exist "%LOCALAPPDATA%\TapirRunner\runner.json" (
    echo  First start: a few questions to set up the runner.
    echo.
    %PY% tapir_test_runner.py setup
    if errorlevel 1 (
        echo.
        echo  Setup did not finish. Read the message above, then double-click this file again.
        pause
        exit /b 1
    )
    echo.
    echo  Setup is done. Opening the runner's window ...
    timeout /t 3 >nul
)
start "" %PYW% tapir_test_runner.py gui
exit /b 0
