cmake_minimum_required (VERSION 3.0)
project ( UWPRunner )

set    ( DAVA_NO_RENDER true )
set    ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include( CMake-common )

dava_add_definitions (
    -DCONSOLE
)

find_package( QT5           REQUIRED )
find_package( DavaFramework REQUIRED )
find_dava_module( LoggerService )
find_dava_module( DocDirSetup )

if ( NOT WIN32 OR WINDOWS_UAP )
    message( FATAL_ERROR "This util can be built only on Windows Desktop platform" )
endif()

dava_add_definitions(
-DQT_NETWORK_LIB
-DQT_CORE_LIB
-DQT_HAVE_MMX
-DQT_HAVE_3DNOW
-DQT_HAVE_SSE
-DQT_HAVE_MMXEXT
-DQT_HAVE_SSE2
-DQT_THREAD_SUPPORT
-D_CRT_SECURE_NO_DEPRECATE
-DDAVA_QT
-DDAVA_FMOD
)

file              ( GLOB UI_LIST "DataQt/*.ui" )
qt5_wrap_ui       ( UI_APP_HDRS  ${UI_LIST} )

file              ( GLOB RCC_LIST "DataQt/*.qrc" )
qt5_add_resources ( RCC_APP_SRCS  ${RCC_LIST} )

include_directories   ( "Classes" )
include_directories   ( "Classes/Qt" )
include_directories   ( "Classes/Qt/corecon" )

set ( Generated          ${UI_APP_HDRS} ${RCC_APP_SRCS} )

define_source ( SOURCE  "Classes"  ${PLATFORM_SPECIFIC_FOLDER} )

list ( APPEND UNIFIED_IGNORE_LIST "Classes/Qt")

set( ADDED_SRC            ${MOC_APP_SRCS} ${UI_APP_HDRS} ${RCC_APP_SRCS}  )
set( LIBRARIES            ${QT_LIBRARIES} runtimeobject.lib XmlLite.lib wsclient.lib shlwapi.lib )
ASSERT(QT_ACTUAL_PATH "Qt path was not resolved")
set( ADDED_BINARY_DIR     ${QT_ACTUAL_PATH}/bin )

setup_main_executable()

set_subsystem_console()
