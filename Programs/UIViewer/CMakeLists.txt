cmake_minimum_required (VERSION 3.0)
project ( UIViewer )

set          ( WARNINGS_AS_ERRORS true )
set          ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include      ( CMake-common )

find_package( DavaFramework REQUIRED COMPONENTS DAVA_DISABLE_AUTOTESTS "Sound" )

find_dava_module( LoggerService )
find_dava_module( DocDirSetup  )
find_dava_module( Version )
find_dava_module( Physics )
find_dava_module( Spine )

include_directories   ( "Sources" )

if( MACOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Mac" )
elseif( IOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Ios" )
elseif( WIN32 )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Win32" )
    set( EXECUTABLE_FLAG WIN32 )
endif()

define_source ( SOURCE "Sources" ${PLATFORM_SPECIFIC_FOLDER} )

set( MIX_APP_DATA                 "Data = ${DAVA_ROOT_DIR}/Programs/Data"
                                  "Data/${PROJECT_NAME} = ${CMAKE_CURRENT_LIST_DIR}/Data" )

set( IOS_PLISTT                 ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/UIVieweriPhone-Info.plist )

set( MACOS_XIB                  ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/MainMenu.xib)
set( MACOS_PLIST                ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/UIViewerMacOS-Info.plist )

set( ADDED_SRC                     ${IOS_ADD_SRC} )

if ( WINDOWS_UAP )
    set ( WIN_STORE_MANIFEST_PACKAGE_GUID "C3B05534-90DF-4E75-829E-25D10C77FE49" )
endif ()

setup_main_executable()