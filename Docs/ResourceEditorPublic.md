# How to build Resource Editor


## MacOS

* Download and install [Qt5.6](https://download.qt.io/archive/qt/5.6/5.6.0/)
* Download and install [Xcode](https://developer.apple.com/xcode/)
* Download and install [Python 2.7](https://www.python.org/downloads/). Add python to ${PATH}
* Copy dava.engine/Sources/CMake/ConfigureFiles/DavaConfigTemplate.in as dava.engine/DavaConfig.in
* Edit DavaConfig.in:
    * setup QT\_PATH: QT\_PATH = /Users/username/sdk/Qt/Qt56/clang_64
* Generate ResourceEditor project:
    * $ dava.engine/Bin/CMakeMac/CMake.app/Contents/bin/cmake **-G "Xcode"** dava.engine/Programs/ResourceEditor -Bdava.engine/Programs/ResourceEditor/\_build **-DPUBLIC\_BUILD=true -DUNITY\_BUILD=true** 
 
 
## Windows
* Download and install [Qt5.6](https://download.qt.io/archive/qt/) 
* Download and install [Visual Studio 2017](https://www.visualstudio.com/downloads/)
* Download and install [Python 2.7](https://www.python.org/downloads/). Add python to ${PATH}
* Download and install DirectX SDK June 2010
* Copy dava.engine/Sources/CMake/ConfigureFiles/DavaConfigTemplate.in as dava.engine/DavaConfig.in
* Edit DavaConfig.in:
    * setup QT\_PATH: QT\_PATH = C:/Qt5.6.0/msvc2017/
* Generate ResourceEditor project:
    * $ C:/dava.engine/Bin/CMakeWin32/bin/cmake.exe **-G "Visual Studio 15 Win64"** C:/dava.engine/Programs/ResourceEditor -BC:/dava.engine/Programs/ResourceEditor/\_build **-DPUBLIC\_BUILD=true -DUNITY\_BUILD=true**


## Addition Note

* To use Qt5.6 with Visual Studio 2017 you have to build it yourself from sources
* We didn't build ResourceEditor with Qt5.8 - Qt5.9 and Visual Studio 2017. But It is possible with small code changes