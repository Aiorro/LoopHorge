@echo off
py -3.11 -I "%~dp0setup-midi-llm.py"
if not errorlevel 1 exit /b 0
py -3.12 -I "%~dp0setup-midi-llm.py"
if not errorlevel 1 exit /b 0
echo Install 64-bit Python 3.11 or 3.12 from python.org with the Python Launcher.
pause
