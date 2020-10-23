@echo off
set SCRIPT_DIR=%~dp0
set CMAKE_DIR=%~dp0cmake_prebuilt
set CMAKE_DIR_BIN=%CMAKE_DIR%\bin

echo SCRIPT_DIR=%SCRIPT_DIR%
echo CMAKE_DIR_BIN=%CMAKE_DIR_BIN%

::cleanup cmake binaries dir and unzip it
if not exist %SCRIPT_DIR%\cmake_prebuilt.zip (echo "Can't find cmake_prebuilt.zip in %CMAKE_DIR%") & (exit /B)
rmdir /s /q %CMAKE_DIR%
call python.exe %SCRIPT_DIR%\unpack.py %SCRIPT_DIR%\cmake_prebuilt.zip %SCRIPT_DIR%

if not exist %CMAKE_DIR_BIN%\cmake.exe (echo "Can't find cmake.exe in %CMAKE_DIR_BIN%") & (exit /B)

@echo on
%CMAKE_DIR_BIN%\cmake.exe %*