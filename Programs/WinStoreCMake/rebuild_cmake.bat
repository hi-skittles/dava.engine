::check existence of repo directory 
@echo off
call update_cmake.bat
set WORK_DIR_REPO=cmake_repo
if not exist %WORK_DIR_REPO% (echo "Can't find %WORK_DIR_REPO%.") & (exit)

::build intermediate cmake
set INTERMEDIATE_DIR_BUILD=cmake_intermediate_build
if exist %INTERMEDIATE_DIR_BUILD% rmdir /s /q %INTERMEDIATE_DIR_BUILD%
mkdir %INTERMEDIATE_DIR_BUILD%
cd %INTERMEDIATE_DIR_BUILD%
cmake ../%WORK_DIR_REPO%
cmake --build . --config release
cd ..

::build final cmake
set WORK_DIR_BUILD=cmake_build
set INTERMEDIATE_CMAKE_EXE=%INTERMEDIATE_DIR_BUILD%\bin\Release\cmake.exe
if exist %WORK_DIR_BUILD% rmdir /s /q %WORK_DIR_BUILD%
mkdir %WORK_DIR_BUILD%
cd %WORK_DIR_BUILD%
..\%INTERMEDIATE_CMAKE_EXE% ../%WORK_DIR_REPO%
..\%INTERMEDIATE_CMAKE_EXE% --build . --config release

::remove intermediate cmake
cd ..
rmdir /s /q %INTERMEDIATE_DIR_BUILD%