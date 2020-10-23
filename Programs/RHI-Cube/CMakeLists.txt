cmake_minimum_required (VERSION 3.0)
project ( RHI-Cube )

set          ( WARNINGS_AS_ERRORS true )
set          ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" ) 
include      ( CMake-common )

find_package( DavaFramework REQUIRED COMPONENTS DAVA_DISABLE_AUTOTESTS )
find_package( ScenePerformanceTests REQUIRED )

include_directories   ( "Classes" )

if( MACOS )
    set( PLATFORM_SPECIFIC_FOLDER "MacOSSpecific" )

elseif( IOS )
    set( PLATFORM_SPECIFIC_FOLDER "iOSSpecific" )
    
elseif( WIN32 )
    set( EXECUTABLE_FLAG WIN32 )

endif()

file ( GLOB_RECURSE XIB_LIST "${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/*.xib" )

define_source ( SOURCE "Classes" ${PLATFORM_SPECIFIC_FOLDER} )

set( APP_DATA                     ${CMAKE_CURRENT_LIST_DIR}/Data )

set( IOS_PLISTT                   ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/RHIcube-Info.plist )

set( MACOS_PLIST                  ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/RHIcube-Info.plist )
set( MACOS_XIB                  ${XIB_LIST} )

set( ANDROID_USE_STANDART_TEMLATE true )
set( ANDROID_PACKAGE              "com.dava.rhicube" )
set( ANDROID_APP_NAME             "RHICubeApp" )
set( ANDROID_ACTIVITY_APP_NAME    "RHICube"   )

set( ANDROID_ICO                ${CMAKE_CURRENT_LIST_DIR}/android/ic_launcher-web.png )

set( ADDED_SRC                  ${IOS_ADD_SRC} )

if ( WINDOWS_UAP )
    set ( WIN_STORE_MANIFEST_PACKAGE_GUID "C3B05534-90DF-4E75-829E-25D10C77FE49" )
endif ()

setup_main_executable()