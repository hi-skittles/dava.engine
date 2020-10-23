@echo off
set START_DIR=%CD%
set SCRIPT_DIR=%~dp0
set SOURCE_DIR=%1/
set CMAKE_DIR=%~dp0cmake_prebuilt
set CMAKE_DIR_BIN=%CMAKE_DIR%\bin

echo START_DIR=%START_DIR%
echo SCRIPT_DIR=%SCRIPT_DIR%
echo SOURCE_DIR=%SOURCE_DIR%
echo CMAKE_DIR_BIN=%CMAKE_DIR_BIN%

::cleanup cmake binaries dir and unzip it
if not exist %SCRIPT_DIR%\cmake_prebuilt.zip (echo "Can't find cmake_prebuilt.zip in %CMAKE_DIR%") & (exit /B)
rmdir /s /q %CMAKE_DIR%
call python.exe %SCRIPT_DIR%\unpack.py %SCRIPT_DIR%\cmake_prebuilt.zip %SCRIPT_DIR%

if not exist %CMAKE_DIR_BIN%\cmake.exe (echo "Can't find cmake.exe in %CMAKE_DIR_BIN%") & (exit /B)

::generate project
if "%1" == "" (echo "Add path which contains CMakeLists.txt") & (exit /B)

if not exist "%SOURCE_DIR%CMakeLists.txt" (echo "Can't find CMakeLists.txt in %SOURCE_DIR%") & (exit /B)

if "%2" == "Win64" set APPEND_A_PLATFORM=-A"x64"
if "%2" == "ARM"   set APPEND_A_PLATFORM=-A"ARM"
if "%2" == "Win32" set APPEND_A_PLATFORM=-A"Win32"
if "%2" == "UnityBuild" set APPEND_UNITY_BUILD=-DUNITY_BUILD=true

if "%3" == "Win64" set APPEND_A_PLATFORM=-A"x64"
if "%3" == "ARM"   set APPEND_A_PLATFORM=-A"ARM"
if "%3" == "Win32" set APPEND_A_PLATFORM=-A"Win32"
if "%3" == "UnityBuild" set APPEND_UNITY_BUILD=-DUNITY_BUILD=true

@echo on
%CMAKE_DIR_BIN%\cmake.exe -G "Visual Studio 14 2015" -DCMAKE_TOOLCHAIN_FILE=%SCRIPT_DIR%/../../Sources/CMake/Toolchains/win_uap.toolchain.cmake %APPEND_A_PLATFORM% %APPEND_UNITY_BUILD% %SOURCE_DIR%