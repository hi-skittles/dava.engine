cmake_minimum_required( VERSION 3.0 )

project               ( TexConverter )

set                   ( WARNINGS_AS_ERRORS true )
set                   ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" ) 
set                   ( NO_CEF true )
include               ( CMake-common )


find_dava_module( TextureCompression )
find_dava_module( DocDirSetup )

dava_add_definitions  ( -DCONSOLE )
find_package          ( DavaFramework REQUIRED COMPONENTS DAVA_DISABLE_AUTOTESTS )

include_directories   ( "Sources" )
define_source ( SOURCE  "Sources" )

set( APP_DATA                    )
set( LIBRARIES                   )

set( MAC_DISABLE_BUNDLE     true )
set( DISABLE_SOUNDS         true)

setup_main_executable()

set_subsystem_console()