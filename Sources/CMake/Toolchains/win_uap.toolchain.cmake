cmake_minimum_required( VERSION 3.2.3 )

set     ( WINDOWS_UAP true )
set     ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../Modules/" ) 
include ( GlobalVariables )

#default target version
set ( WINDOWS_UAP_DEFAULT_TARGET_PLATFORM_VERSION "10.0.10240.0" )
if ( NOT WINDOWS_UAP_TARGET_PLATFORM_VERSION )
    set( WINDOWS_UAP_TARGET_PLATFORM_VERSION ${WINDOWS_UAP_DEFAULT_TARGET_PLATFORM_VERSION} )
endif()

#define system name and version for windows universal application
set ( CMAKE_SYSTEM_NAME "WindowsStore" )
set ( CMAKE_SYSTEM_VERSION ${WINDOWS_UAP_TARGET_PLATFORM_VERSION} )

#project platforms
#if concrete platform not set, project will be multiplatform
#else project will be singleplatform
if ( NOT CMAKE_GENERATOR_PLATFORM )
    set ( CMAKE_VS_EFFECTIVE_PLATFORMS "Win32;ARM;x64" )
    set ( WINDOWS_UAP_MULTIPLATFORM true )
    set ( WINDOWS_UAP_PLATFORMS ${CMAKE_VS_EFFECTIVE_PLATFORMS} )
else ()
    set ( WINDOWS_UAP_PLATFORMS ${CMAKE_GENERATOR_PLATFORM} )
endif ()