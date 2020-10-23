cmake_minimum_required (VERSION 3.0)

set                   ( CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebinfo" CACHE STRING "limited configs" FORCE )

project               ( ResourceEditor )

get_filename_component( FRAMEWORK_PATH "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
get_filename_component( RE_PATH "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

set (ADD_TERRAIN_PLUGIN false)
if (ADD_TERRAIN_PLUGIN)
set                   ( WARNINGS_AS_ERRORS false )
else()
set                   ( WARNINGS_AS_ERRORS true )
endif()
set                   ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FRAMEWORK_PATH}/Sources/CMake/Modules/" )
set                   ( TOOL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

include ( CMake-common )

find_package( GMock REQUIRED )
find_package( QT5 REQUIRED )

find_dava_module( REPlatform )
find_dava_module( Spine )
find_dava_module( DocDirSetup )
find_dava_module( RemoteTool )
find_dava_module( Physics )
find_dava_module( PhysicsDebug )
find_dava_module( AssetCache )
find_dava_module( TexturePacker )
find_dava_module( TextureCompression )
find_dava_module( Version )
find_dava_module( FBX )


if (NOT PUBLIC_BUILD)
    find_dava_module( SpeedTree )
    find_dava_module( Beast )
endif()

list( APPEND DAVA_COMPONENTS Sound )
list( APPEND DAVA_COMPONENTS DAVA_PLATFORM_QT )
list( APPEND DAVA_COMPONENTS DAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME )
list( APPEND DAVA_COMPONENTS DAVA_DISABLE_AUTOTESTS )
list( APPEND DAVA_COMPONENTS DAVA_USE_RENDERSTATS )

find_package(DavaFramework REQUIRED COMPONENTS ${DAVA_COMPONENTS})

if (WIN32 AND ADD_TERRAIN_PLUGIN)
  add_plugin(TerrainConverter "${FRAMEWORK_PATH}/Programs/REPlugins/TerrainConverter")
endif()

include_directories (${CMAKE_CURRENT_BINARY_DIR})

include_directories (${DAVA_ROOT_DIR}/Programs/ColladaConverter/Collada15/FCollada)
include_directories (${DAVA_ROOT_DIR}/Programs/ColladaConverter/Collada15/External/Cg/include)

include_directories (${DAVA_THIRD_PARTY_ROOT_PATH})
include_directories (${DAVA_THIRD_PARTY_ROOT_PATH}/bullet)

dava_add_definitions       (
-D__DAVAENGINE_RENDERSTATS__
-DQT_OPENGL_LIB
-DQT_GUI_LIB
-DQT_NETWORK_LIB
-DQT_CORE_LIB
-DQT_HAVE_MMX
-DQT_HAVE_3DNOW
-DQT_HAVE_SSE
-DQT_HAVE_MMXEXT
-DQT_HAVE_SSE2
-DQT_THREAD_SUPPORT
-D_CRT_SECURE_NO_DEPRECATE
-D__DAVAENGINE_RESOURCEEDITOR__
-DDAVA_QT
)

if (DEPLOY)
    dava_add_definitions(-DDEPLOY_BUILD)
endif()

file              ( GLOB UI_LIST "DataQt/*.ui" )
qt5_wrap_ui       ( UI_APP_HDRS  ${UI_LIST} )

file              ( GLOB RCC_LIST "DataQt/*.qrc" )
qt5_add_resources ( RCC_APP_SRCS  ${RCC_LIST} )

if( WIN32 )
    set( THIRD_PARTY_LIB_RELEASE  ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Release/bullet.lib )
    set( THIRD_PARTY_LIB_DEBUG    ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Debug/bullet.lib )
elseif( MACOS )
    set( THIRD_PARTY_LIB_RELEASE ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/libbullet.a )
    set( THIRD_PARTY_LIB_DEBUG ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/libbullet.a )
endif()

set ( Generated          ${UI_APP_HDRS} ${RCC_APP_SRCS} )

define_source ( SOURCE "Classes" "DataQt" GROUP_SOURCE Generated )

set( MIX_APP_DATA         "Data = ${DAVA_ROOT_DIR}/Programs/Data" 
                          "Data/${PROJECT_NAME} = ${CMAKE_CURRENT_LIST_DIR}/Data" )

set( MACOS_PLIST          ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/Info.plist )
set( MACOS_ICO            ${CMAKE_CURRENT_LIST_DIR}/MacOSSpecific/icon.icns  )
set( ADDED_SRC            ${UI_APP_HDRS} ${RCC_APP_SRCS} ${QML_LIST} )
set( WIN32_RESOURCES            ${CMAKE_CURRENT_LIST_DIR}/WinSpecific/Resources.rc )

set( LIBRARIES_RELEASE    ${THIRD_PARTY_LIB_RELEASE} )
set( LIBRARIES_DEBUG      ${THIRD_PARTY_LIB_DEBUG}   )

ASSERT(QT_ACTUAL_PATH "Qt path was not resolved")
set( ADDED_BINARY_DIR     ${QT_ACTUAL_PATH}/bin )
set( UNIFIED_IGNORE_LIST "Classes/Collada" )

convert_graphics()

setup_main_executable()
