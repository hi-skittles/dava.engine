cmake_minimum_required (VERSION 3.0)
project               ( AssetCacheServer )

set                   ( WARNINGS_AS_ERRORS true )
set                   ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" )
include               ( CMake-common )

find_package          ( QT5 REQUIRED )
find_dava_module   ( QtHelpers )
find_dava_module   ( AssetCache )
find_dava_module   ( Version )
find_dava_module   ( DocDirSetup )

dava_add_definitions  ( -DCONSOLE )

find_package          ( DavaFramework REQUIRED COMPONENTS DAVA_DISABLE_AUTOTESTS )

include_directories   ( ${CMAKE_CURRENT_BINARY_DIR} )

include_directories   ( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories   (.)
include_directories   ( "Classes" )

dava_add_definitions       (
-DQT_GUI_LIB
-DQT_NETWORK_LIB
-DQT_CORE_LIB
-DQT_HAVE_MMX
-DQT_HAVE_3DNOW
-DQT_HAVE_SSE
-DQT_HAVE_MMXEXT
-DQT_HAVE_SSE2
-DQT_THREAD_SUPPORT
-DDAVA_QT
)

file              ( GLOB UI_LIST "DataQt/*.ui" )
qt5_wrap_ui       ( UI_APP_HDRS  ${UI_LIST} )

file              ( GLOB RCC_LIST "DataQt/*.qrc" )
qt5_add_resources ( RCC_APP_SRCS  ${RCC_LIST} )

set               ( Generated ${UI_APP_HDRS} ${RCC_APP_SRCS} )
set               ( UiList ${UI_LIST} )

define_source ( SOURCE    "Classes" ${PLATFORM_SPECIFIC_FOLDER} GROUP_SOURCE Generated UiList )

#set( APP_DATA             ${CMAKE_CURRENT_LIST_DIR}/Data )
set( MACOS_PLIST          ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/Info.plist )
set( MACOS_ICO            ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/icon.icns )
set( ADDED_SRC            ${UI_APP_HDRS} ${RCC_APP_SRCS} )
ASSERT(QT_ACTUAL_PATH "Qt path was not resolved")
set( ADDED_BINARY_DIR     ${QT_ACTUAL_PATH}/bin )

set( DISABLE_SOUNDS         true)

setup_main_executable()
