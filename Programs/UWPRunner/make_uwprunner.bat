::clean build directory
rmdir /s /q _build
mkdir _build
cd _build

::generate project and building
cmake -G"Visual Studio 12 Win64" ../ -DUNITY_BUILD=true
cmake --build . --config Release

::leave directory and copy artifacts to Tools/Bin
cd ..
copy /Y _build\Release\UWPRunner.exe ..\Bin\x64