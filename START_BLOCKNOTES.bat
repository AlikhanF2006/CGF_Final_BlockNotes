@echo off
cd /d "%~dp0"
"windows\BlockNotes.exe"
if errorlevel 1 pause
