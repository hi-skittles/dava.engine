cmake_minimum_required( VERSION 3.0 )

project               ( FontGenerator )

#set                   ( WARNINGS_AS_ERRORS true )
set                   ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include               ( CMake-common )

dava_add_definitions  ( -DCONSOLE)
find_package          ( DavaFramework REQUIRED COMPONENTS DAVA_DISABLE_AUTOTESTS )

include_directories   ( "Classes" "ThirdParty" )

define_source ( SOURCE "Classes" "ThirdParty" )

set( MACOS_PLIST          ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/Info.plist )

set( APP_DATA )
set( LIBRARIES )

set( MAC_DISABLE_BUNDLE     true )
set( DISABLE_SOUNDS         true )

setup_main_executable()
set_subsystem_console()


