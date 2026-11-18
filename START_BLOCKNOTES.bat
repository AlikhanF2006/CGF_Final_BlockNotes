@echo off
cd /d "%~dp0"
"windows\BlockNotes.exe" --demo
if errorlevel 1 pause
