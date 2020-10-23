cmake_minimum_required (VERSION 3.0)

project ( TestServer )

set          ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include      ( CMake-common )


set ( DAVA_COMPONENTS "NetworkCore" DAVA_DISABLE_AUTOTESTS )

dava_add_definitions(-DDISABLE_NATIVE_MOVIEVIEW)
dava_add_definitions(-DDISABLE_NATIVE_TEXTFIELD)
dava_add_definitions(-DDISABLE_NATIVE_WEBVIEW)

find_package( DavaFramework REQUIRED COMPONENTS "${DAVA_COMPONENTS}" )

include_directories   ( "Classes" )

if( MACOS )
    set( PLATFORM_SPECIFIC_FOLDER "MacOSSpecific" )
elseif( WIN32 )
    set( PLATFORM_SPECIFIC_FOLDER "Win32Specific" )
    set( EXECUTABLE_FLAG WIN32 )
endif()

define_source                  ( SOURCE "Classes" )

set( MIX_APP_DATA                 "Data = ${DAVA_ROOT_DIR}/Programs/Data" 
                                  "Data = ${CMAKE_CURRENT_LIST_DIR}/Data" )

set( MACOS_XIB                  ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/MainMenu.xib)
set( MACOS_PLIST                ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/Info.plist )

setup_main_executable()
convert_graphics()
