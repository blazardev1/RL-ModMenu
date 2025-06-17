@echo off
setlocal enabledelayedexpansion

:: Create build directory if it doesn't exist
if not exist "build" mkdir build

:: Configure CMake
cmake -B build -G "Visual Studio 17 2022" -A x64

:: Build the project
cmake --build build --config Release

:: Copy required files to the build directory
xcopy /Y "discord-rpc.dll" "build\bin\"

echo.
echo Build completed! Check the build\bin directory for the output files.
