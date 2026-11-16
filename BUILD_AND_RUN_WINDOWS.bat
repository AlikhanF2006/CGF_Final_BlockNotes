@echo off
cd /d "%~dp0"
cmake -S . -B build
if errorlevel 1 goto fail
cmake --build build --config Release
if errorlevel 1 goto fail
if exist "build\Release\BlockNotes.exe" (
    "build\Release\BlockNotes.exe" --demo
) else (
    "build\BlockNotes.exe" --demo
)
if errorlevel 1 goto fail
exit /b 0
:fail
echo Build or launch failed. Read README.md for the required C++ and CMake tools.
pause
exit /b 1
