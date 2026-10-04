@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b %errorlevel%

cmake --preset x64-debug
if errorlevel 1 exit /b 1
cmake --build --preset x64-debug
if errorlevel 1 exit /b 1
if /I "%1"=="test" ctest --test-dir out\build\x64-debug --output-on-failure
exit /b 0
