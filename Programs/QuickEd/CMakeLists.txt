cmake_minimum_required (VERSION 3.0) 

set                   ( CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebinfo" CACHE STRING "limited configs" FORCE )

project               ( QuickEd )

get_filename_component( FRAMEWORK_PATH "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

set                   ( WARNINGS_AS_ERRORS true )
set                   ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FRAMEWORK_PATH}/Sources/CMake/Modules/" )
set                   ( TOOL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

include               ( CMake-common )

if (DEPLOY)
    set (DEPLOY_ROOT ${DEPLOY_DIR})

    if ( WIN32 )
        set(DEPLOY_ROOT ${DEPLOY_ROOT})
        set(DEPLOY_DIR ${DEPLOY_ROOT})
    endif()
    set (TOOL_OUTPUT_DIR ${DEPLOY_DIR})
endif()

find_package( QT5 REQUIRED )

find_dava_module( AssetCache )
find_dava_module( TextureCompression )
find_dava_module( TexturePacker )
find_dava_module( Version )
find_dava_module( QtHelpers )
find_dava_module( QtTools )
find_dava_module( DocDirSetup )
find_dava_module( Spine )
find_dava_module( TArc )
find_dava_module( RemoteTool )

list( APPEND DAVA_COMPONENTS Sound )
list( APPEND DAVA_COMPONENTS DAVA_PLATFORM_QT )
list( APPEND DAVA_COMPONENTS DAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME )
list( APPEND DAVA_COMPONENTS DAVA_DISABLE_AUTOTESTS )
list( APPEND DAVA_COMPONENTS DAVA_USE_RENDERSTATS )
list( APPEND DAVA_COMPONENTS DAVA_LOCALIZATION_DEBUG )

find_package( DavaFramework REQUIRED COMPONENTS ${DAVA_COMPONENTS} )

include_directories   ( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories   (.)
include_directories   ( "Classes" )
include_directories   ( "Classes/Helpers" )
include_directories   ( "Classes/Controllers" )
include_directories   ( "Classes/Models" )

if (DEPLOY)
    dava_add_definitions(-DDEPLOY_BUILD)
endif()


dava_add_definitions       ( 
-DQT_OPENGL_LIB
-DQT_GUI_LIB
-DQT_CORE_LIB
-DDAVA_QT
-DQT_HAVE_MMX
-DQT_HAVE_3DNOW
-DQT_HAVE_SSE
-DQT_HAVE_MMXEXT
-DQT_HAVE_SSE2
-DQT_THREAD_SUPPORT

-D__DAVAENGINE_RENDERSTATS__
-D_CRT_SECURE_NO_DEPRECATE
-DDAVA_FMOD
-DQT_NETWORK_LIB
)


file                          ( GLOB UI_LIST "Ui/*.ui" )
qt5_wrap_ui                   ( UI_APP_HDRS  ${UI_LIST} )

file                          ( GLOB RCC_LIST "DataQt/*.qrc" )
qt5_add_resources             ( RCC_APP_SRCS  ${RCC_LIST} )

file                          ( GLOB QML_LIST "DataQt/*.qml")

set                           ( Generated    ${UI_APP_HDRS} ${RCC_APP_SRCS} )
set                           ( UiList       ${UI_LIST} )

define_source ( SOURCE  "Classes"  GROUP_SOURCE Generated UiList )

set( MIX_APP_DATA         "Data = ${DAVA_ROOT_DIR}/Programs/Data" 
                          "Data/${PROJECT_NAME} = ${CMAKE_CURRENT_LIST_DIR}/Data" )

set( MACOS_PLIST                ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/Info.plist )
set( MACOS_ICO                  ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/icon.icns )
set( ADDED_SRC                  ${UI_APP_HDRS} ${RCC_APP_SRCS} ${QML_LIST})
set( WIN32_RESOURCES            ${CMAKE_CURRENT_LIST_DIR}/WinSpecific/Resources.rc )

ASSERT(QT_ACTUAL_PATH "Qt path was not resolved")
set( ADDED_BINARY_DIR     ${QT_ACTUAL_PATH}/bin )

convert_graphics()
setup_main_executable()

