cmake_minimum_required (VERSION 3.0)
project ( TestBed )

# Enable MemoryManagerTest in UnitTests
# this variable should be defined in all dependant projects (dava framework, etc)
if( NOT DAVA_MEGASOLUTION )
    #set ( DAVA_MEMORY_PROFILER 1 )
endif()

set          ( WARNINGS_AS_ERRORS true )
set          ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include      ( CMake-common  )

if (LINUX)
    dava_add_definitions(-DDISABLE_NATIVE_MOVIEVIEW)
    dava_add_definitions(-DDISABLE_NATIVE_TEXTFIELD)
    dava_add_definitions(-DDISABLE_NATIVE_WEBVIEW)
endif()

# Uncomment to use local resources for android
# dava_add_definitions(-DUSE_LOCAL_RESOURCES)

set ( DAVA_COMPONENTS  "DAVA_DISABLE_AUTOTESTS" )
find_dava_module( SampleModule )

if (NOT LINUX)
    list(APPEND DAVA_COMPONENTS Sound )
    find_dava_module( Spine )
endif()

find_dava_module( CEFWebview )

# add physics

find_dava_module( Physics )
find_dava_module( PhysicsDebug )
find_dava_module( Version )

list( APPEND DAVA_COMPONENTS DAVA_LOCALIZATION_DEBUG )


if( NOT WINDOWS_UAP AND ( MACOS OR WIN32 ) )
    add_plugin( SamplePlugin "${DAVA_MODULES_DIR}/SamplePlugin" COMPONENTS  ${DAVA_COMPONENTS} )
endif()


find_dava_module( LoggerService )
find_dava_module( MemoryProfilerService )
find_dava_module( EmbeddedWebServer )
find_dava_module( DocDirSetup )

find_package( DavaFramework REQUIRED COMPONENTS ${DAVA_COMPONENTS} )


include_directories   ( "Sources" )
include_directories   ( "Sources/Infrastructure" )

if( MACOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Mac" )
elseif( IOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Ios" )
    set( IOS_ADD_SRC ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/TestBed.entitlements )
elseif( WIN32 )
    set( EXECUTABLE_FLAG WIN32 )
endif()

file ( GLOB_RECURSE XIB_LIST "${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/*.xib" )

define_source ( SOURCE    "Sources" )

set( MIX_APP_DATA         "Data = ${DAVA_ROOT_DIR}/Programs/Data" 
                          "Data/${PROJECT_NAME} = ${CMAKE_CURRENT_LIST_DIR}/Data" )

if (MACOS OR WIN32)
    set( APP_DATA  ${APP_DATA} "scripts")
endif()

set( IOS_PLISTT                 ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/TestBed-Info.plist )

set( MACOS_PLIST                ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/Info.plist )
set( MACOS_XIB                  ${XIB_LIST} )

set( ANDROID_USE_STANDART_TEMLATE true )
set( ANDROID_PACKAGE            "com.dava.testbed" )
set( ANDROID_APP_NAME           "TestBedApp" )
set( ANDROID_ACTIVITY_APP_NAME  "TestBed"   )

set( ANDROID_ICO                ${CMAKE_CURRENT_LIST_DIR}/android/ic_launcher-web.png )

set( ADDED_SRC                  ${IOS_ADD_SRC} )
set( LIBRARIES                  )

if ( WINDOWS_UAP )
    set ( WIN_STORE_MANIFEST_PACKAGE_GUID "5B2576F2-B514-432B-BD8B-0665D4F96BC2" )
endif ()

if (ANDROID)
    # Libraries and classes to load at startup
    set (ANDROID_BOOT_MODULES "c++_shared;fmodex;fmodevent;TestBed")
    set (ANDROID_BOOT_CLASSES "com.dava.testbed.TestBed")
endif()

setup_main_executable()
convert_graphics()


if (IOS)
    # Termporal workaround for unit tests with memory profiling enabled
    # Reason: on iOS on some circumstances memory deallocating operation bypasses memory manager
    set_xcode_property( ${PROJECT_NAME} STRIP_INSTALLED_PRODUCT NO )
endif()

dava_pre_build_step(NAME generate_local_meta_data
                    COMMAND python ARGS ${CMAKE_CURRENT_LIST_DIR}/Scripts/build_local_meta.py
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
