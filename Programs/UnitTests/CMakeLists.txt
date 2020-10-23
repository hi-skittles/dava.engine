cmake_minimum_required (VERSION 3.0)

set( COVERAGE true )

project ( UnitTests )

set          ( WARNINGS_AS_ERRORS true )
set          ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include      ( CMake-common )

# Enable MemoryManagerTest in UnitTests
# this variable should be defined in all dependant projects (dava framework, etc)
if( NOT DAVA_MEGASOLUTION AND NOT DISABLE_MEMORY_PROFILER )
    # WARNING !!! memory profiler crush on win64 build (but UnitTests works on 32bit in all our tests)
    set ( DAVA_MEMORY_PROFILER 1 )
endif()

if (LINUX)
    set ( DAVA_MEMORY_PROFILER 0 )
endif()

# Enable LOCALIZATION_DEBUG in UnitTests to verify successful compilation
dava_add_definitions(-DLOCALIZATION_DEBUG)

if (LINUX)
    dava_add_definitions(-DDISABLE_NATIVE_MOVIEVIEW)
    dava_add_definitions(-DDISABLE_NATIVE_TEXTFIELD)
    dava_add_definitions(-DDISABLE_NATIVE_WEBVIEW)
endif()

if (NOT LINUX)
    # Enable sound after fmod libraries
    set ( DAVA_COMPONENTS Sound )
    find_dava_module( Spine )
    find_dava_module( NetworkCore )
endif()

find_dava_module( AssetCache )			# supported platforms are defined in module
find_dava_module( ResourceArchiverModule )	# supported platforms are defined in module
find_dava_module( TextureCompression )		# supported platforms are defined in module
find_dava_module( TexturePacker )		# supported platforms are defined in module


find_dava_module( Physics )
find_dava_module( CEFWebview )
find_dava_module( Sample )
find_dava_module( LoggerService  )
find_dava_module( EmbeddedWebServer  )
find_dava_module( DocDirSetup  )
find_dava_module( Version )

find_package( Steam REQUIRED )


find_package( DavaFramework REQUIRED COMPONENTS "${DAVA_COMPONENTS}" DAVA_DISABLE_AUTOTESTS )


include_directories   ( "Sources" )

if( MACOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Mac" )

elseif( IOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Ios" )
    set( IOS_ADD_SRC ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/UnitTests.entitlements )

elseif( WIN32 )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Win32" )
    set( EXECUTABLE_FLAG WIN32 )

endif()

define_source                  ( SOURCE "Sources" )

set( STEAM_APPID                Platforms/Win32/steam_appid.txt )

set( MIX_APP_DATA                 "Data = ${DAVA_ROOT_DIR}/Programs/Data" 
                                  "Data = ${CMAKE_CURRENT_LIST_DIR}/Data" )

set( IOS_PLISTT                 ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/UnitTests-Info.plist )

set( MACOS_XIB                  ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/MainMenu.xib)
set( MACOS_PLIST                ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/Info.plist )

set( ADDED_SRC                  ${IOS_ADD_SRC} )

#uncomment this 2 strings to link libjpeg as additional project.
#set( LIBRARIES jpeg )
#add_subdirectory ( "${CMAKE_CURRENT_LIST_DIR}/../../Libs/libjpeg" ${CMAKE_CURRENT_BINARY_DIR}/libjpeg )

if ( WINDOWS_UAP )
    set ( WIN_STORE_MANIFEST_PACKAGE_GUID "49484B77-9BB6-4FBC-9D56-77593EF55C45" )
endif ()

if (ANDROID)
    # Libraries and classes to load at startup
    set (ANDROID_BOOT_MODULES "c++_shared;fmodex;fmodevent;UnitTests")
    set (ANDROID_BOOT_CLASSES "com.dava.unittests.UnitTests")
endif()

exctact_external_unittests()
setup_main_executable()
convert_graphics()
 
if (IOS)
    set_xcode_property( ${PROJECT_NAME} ONLY_ACTIVE_ARCH YES )

    # Termporal workaround for unit tests with memory profiling enabled
    # Reason: on iOS on some circumstances memory deallocating operation bypasses memory manager
    set_xcode_property( ${PROJECT_NAME} STRIP_INSTALLED_PRODUCT NO )
endif()
