@echo off
setlocal

@rem Check Python
where python >nul 2>nul
if errorlevel 1 (
    echo ERROR: Python is not found in PATH.
    pause
    exit /b 1
)

@rem Check pioasm.exe
if not exist "%~dp0pioasm.exe" (
    echo ERROR: pioasm.exe not found in current directory: %~dp0
    pause
    exit /b 1
)

@rem Run
pioasm.exe program.pio > program.h
if errorlevel 1 (
    echo ERROR: Failed to run pioasm.exe
    pause
    exit /b 1
)

python pio_settings_ini_gen.py program.h > settings.ini
if errorlevel 1 (
    echo ERROR: Failed to run Python script
    pause
    exit /b 1
)

echo All steps completed successfully.
pause
endlocal
