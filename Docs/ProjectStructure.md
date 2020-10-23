# Project Structure

## Basic files/folders naming requirements
- Files and folders are named using CamelCase, while some exceptions may apply (see below).
- File or folder related to a particular platform must be named using the following convention: _Name\<Platform\>_. For example `DeviceInfoWin32.cpp`, where _Name_ is `DeviceInfo` and _Platform_ is `Win32`. Folders can also be named with a short form, when _Name_ is empty, e.g. `Android/`. 

    _Platform_ have to be one of the following:
    - Android
    - Mac
    - Ios
    - Win32
    - Win10
    - Linux

- File or folder related to multiple platforms can be named using the following convention: _Name\<PlatformGroup\>_, e.g. `ThreadPosix.cpp`, where _Name_ is `Thread` and _PlatformGroup_ is `Posix`. _PlatformGroup_ can be one of the following:
  - Posix
  - Windows
  - Apple

- We support several processor architectures. If the directory is related to a particular architecture, then this relation must be expressed in directory naming in the following form: _Name\<Architecture\>_, where _Architecture_ is one of the following:
  - x86
  - x64
  - arm
  - arm64
- More..

## Top-level directory layout
TODO: add some description

    .
    |- Bin/
    |- Docs/
    |- Libs/
    |- Modules/
    |- Programs/
    |- Sources/
    |- LICENSE
    |- README.md

## Bin

## Doc
It is used to store some reference data about the project, such documentation, best practive guids, codestyle, etc.
    
    |- ...
    |- Docs/
    |  |- Doxygen/
    |  |- Codestyle.md
    |  |- Structure.md
    |  |- FAQ.md
    |  |- ...
    |- ...

## Libs
    |- ...
    |- Libs/
    |  |- Build/
    |  |- Include/
    |  |- Lib/
    |- ...

#### Libs/Build
    |- ...
    |- Build/
    |  |- liba/
    |  |  |- build.py
    |  |- libb/
    |  |  |- build.py
    |  |- ...
    |  |- libz/
    |  |  |- build.py
    |  |- build.py
    |- ...
    
#### Libs/Include
    |- ...
    |- Include/
    |  |- liba/
    |  |  |- a.h
    |  |- libb/
    |  |  |- b.h
    |  |- ...
    |  |- libz/
    |  |  |- z.h
    |- ...
    
#### Libs/Lib

## Sources

#### Sources/Internal
#### Sources/External

## Modules
    |- ...
    |- Modules
    |  |- ModuleA
    |  |- ModuleB
    |  |- ...
    |  |- ModuleZ
    |- ...

#### Modules/X

    |- ...
    |- Modules/
    |  |- ModuleX/
    |  |  |- Docs/
    |  |  |- Libs/
    |  |  |- Platforms/
    |  |  |- Sources/
    |  |  |  |- Private/
    |  |  |  |- ModuleName.h
    |  |  |- README.md
    |  |- ...
    |- ...

#### Modules/X/Platforms/Android
    |- ...
    |- ModuleX
    |  |- ...
    |  |- Platforms/
    |  |  |- Android
    |  |  |  |- libs/
    |  |  |  |- src/
    |  |  |  |  |- main
    |  |  |  |  |  |- java
    |  |  |  |  |  |  |- com
    |  |  |  |  |  |  |  |- dava
    |  |  |  |  |  |  |  |  |- modules
    |  |  |  |  |  |  |  |  |  |- ModuleX
    |  |  |  |  |  |  |  |  |  |  | - ModuleX.java
    |  |  |  |- AndroidManifest.xml
    |  |  |  |- build.gradle
    |  |  |- 

#### Modules/X/Platform/Ios

## Programs
    |- ...
    |- Programs
    |  |- ProgramA
    |  |- ProgramB
    |  |- ...
    |  |- ProgramZ
    |- ...

#### Programs/X

## User Projects

## Appendix A: Full folders structure
    .
    |- Bin/
    |  |- bin1.exe
    |  |- bin1
    |  |- ...
    |  |- binN.exe
    |  |- binN
    |- Docs/
    |  |- Doxygen/
    |  |  |- doxyfile
    |  |- Codestyle.md
    |  |- FAQ.md
    |  |- ...
    |  |- Structure.md
    |- Libs/
    |  |- Build
    |  |  |- liba/
    |  |  |  |- build.py
    |  |  |- libb/
    |  |  |  |- build.py
    |  |  |- ...
    |  |  |- libz/
    |  |  |  |- build.py
    |  |  |- build.py
    |  |- Include
    |  |  |- liba/
    |  |  |  |- a.h
    |  |  |- libb/
    |  |  |  |- b.h
    |  |  |- ...
    |  |  |- libz/
    |  |  |  |- z.h
    |  |- Lib
    |- Modules/
    |  |- ModuleX/
    |  |  |- Docs/
    |  |  |- Libs/
    |  |  |- Platforms/
    |  |  |  |- Android
    |  |  |  |  |- libs/
    |  |  |  |  |- src/
    |  |  |  |  |  |- main
    |  |  |  |  |  |  |- java
    |  |  |  |  |  |  |  |- com
    |  |  |  |  |  |  |  |  |- dava
    |  |  |  |  |  |  |  |  |  |- modules
    |  |  |  |  |  |  |  |  |  |  |- ModuleX
    |  |  |  |  |  |  |  |  |  |  |  | - ModuleX.java
    |  |  |  |  |- AndroidManifest.xml
    |  |  |  |  |- build.gradle
    |  |  |  | - Ios
    |  |  |  |  | - Entitlements.plist
    |  |  |  |  | - ModuleX.entitlements
    |  |  |  |  | - ModuleX.plist
    |  |  |  |  | - ModuleX-Info.plist
    |  |  |  | - ...
    |  |  |- Sources/
    |  |  |  |- Private/
    |  |  |  |  | - ModuleXImpl.cpp
    |  |  |  |  | - ...
    |  |  |  |- ModuleX.h
    |  |  |- README.md
    |  |- ...
    |- Programs/
    |  |- ProgramA/
    |  |- ...
    |  |- ProgramZ/
    |  |  |- ...
    |- Sources/
    |- LICENSE
    |- README.md
