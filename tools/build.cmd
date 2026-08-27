@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b %errorlevel%

if /I "%1"=="configure" (
    cmake -S . -B out\build\x64-Debug -DBORION_BUILD_TESTS=ON
    if errorlevel 1 exit /b 1
    exit /b 0
)

cmake --build out\build\x64-Debug --config Debug --parallel
if errorlevel 1 exit /b 1
exit /b 0
