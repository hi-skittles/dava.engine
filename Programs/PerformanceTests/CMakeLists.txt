cmake_minimum_required (VERSION 3.0)

project      ( PerformanceTests )

set          ( WARNINGS_AS_ERRORS true )
set          ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../../Sources/CMake/Modules/" ) 
include      ( CMake-common )

if ( ENABLE_MEM_PROFILING )
    dava_add_definitions( -DDAVA_MEMORY_PROFILING_ENABLE )
endif()

set ( DAVA_COMPONENTS DAVA_DISABLE_AUTOTESTS )

find_dava_module( Sample )

if (NOT LINUX)
    list ( APPEND DAVA_COMPONENTS Sound  )
    find_dava_module( Spine )
endif()

find_dava_module( CEFWebview )
find_dava_module( Version )


# add physics
find_dava_module( Physics )
find_dava_module( PhysicsDebug )

#without this line another unique instance of DAVA will be created
list( APPEND DAVA_COMPONENTS DAVA_LOCALIZATION_DEBUG )

find_package( DavaFramework REQUIRED COMPONENTS ${DAVA_COMPONENTS} )


include_directories   ( "Sources" )
include_directories   ( "Sources/Infrastructure" )
include_directories   ( "Sources/Controller" )
include_directories   ( "Sources/Screen" )
include_directories   ( ${DAVA_INCLUDE_DIR} ) 
include_directories   ( ${DAVA_THIRD_PARTY_INCLUDES_PATH} )

set( PackerTools             ${DAVA_TOOLS_DIR}/TexturePacker )
set( CommandLineTools        ${DAVA_TOOLS_DIR}/CommandLine/CommandLineParser.h 
                             ${DAVA_TOOLS_DIR}/CommandLine/CommandLineParser.cpp )

if( MACOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Mac" )
elseif( IOS )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Ios" )
elseif( WIN32 AND NOT WINDOWS_UAP )
    set( PLATFORM_SPECIFIC_FOLDER "Platforms/Win32" )
    set( EXECUTABLE_FLAG WIN32 )
endif()

define_source ( SOURCE "Sources" ${PLATFORM_SPECIFIC_FOLDER} ${TeamcityOutput} )


if ( WINDOWS_UAP )
    set ( WIN_STORE_MANIFEST_PACKAGE_GUID "31a2f61a-4083-4250-9c0a-2d68740285b3" )
endif ()


set( APP_DATA             ${CMAKE_CURRENT_LIST_DIR}/Data )

if (NOT PERFORMANCE_TEST_DATA_SVN)
    set( PERFORMANCE_TEST_DATA_SVN ${DAVA_ROOT_DIR}/../performance.test)
endif()

set( MACOS_XIB            ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/MainMenu.xib )
set( MACOS_PLIST          ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/Info.plist )
set( IOS_PLISTT           ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_SPECIFIC_FOLDER}/Info.plist )

set( ADDED_SRC            ${IOS_ADD_SRC})
set( LIBRARIES            )
set( LIBRARIES_RELEASE    )
set( LIBRARIES_DEBUG      )
set( ADDED_BINARY_DIR     )

if( NOT DAVA_MEGASOLUTION )
#[[ do not run automatic resource convertation
    if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
        set( RES_EDITOR_BINARY   "${RES_EDITOR_PATH}/ResourceEditor.exe")       
    else()            
        set( RES_EDITOR_BINARY   "${RES_EDITOR_PATH}/ResourceEditor.app/Contents/MacOS/ResourceEditor")
    endif()

    set (TOOLS_DATA ${DAVA_ROOT_DIR}/Programs/Data)

    message( "--Performance test data folder: " ${PERFORMANCE_TEST_DATA_SVN} )

    if( NOT EXISTS ${PERFORMANCE_TEST_DATA_SVN})
        message( "Error !!!: Please checkout performance.test"  )
        message( " " )
        exit()
    endif()

    if( NOT RES_EDITOR_PATH )
        message( "Error !!!: Please set the correct path 'RES_EDITOR_PATH' to Resource editor in file DavaConfig.in"  )
        message( " " )
        exit()
    endif()

    if( NOT GPU_PARAM)
        set(GPU_PARAM   "PowerVR_iOS")
    endif()


    configure_file( ${CMAKE_CURRENT_LIST_DIR}/scripts/TemplateConvert3D.in
                        ${CMAKE_CURRENT_LIST_DIR}/DataSource/convert_3d.py @ONLY )

    configure_file( ${CMAKE_CURRENT_LIST_DIR}/scripts/TemplateConvert3D_FX.in
                        ${CMAKE_CURRENT_LIST_DIR}/DataSource/convert_3d_FX.py @ONLY )

    configure_file( ${CMAKE_CURRENT_LIST_DIR}/scripts/TemplateConvert3DTanks.in
                        ${CMAKE_CURRENT_LIST_DIR}/DataSource/convert_3d_tanks.py @ONLY )

    message( "============================ Copy resources ============================"  )

    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${TOOLS_DATA}/Materials ${APP_DATA}/Materials )

    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${PERFORMANCE_TEST_DATA_SVN}/Data ${APP_DATA} )
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${PERFORMANCE_TEST_DATA_SVN}/DataSource ${APP_DATA}/../DataSource )

    message( "============================Convert resources ============================"  )

    execute_process( COMMAND python convert_everything.py --gpu ${GPU_PARAM} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/DataSource )
    execute_process( COMMAND python create_file_structure.py WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/DataSource )
]]
endif()

setup_main_executable()
