include ( CMake-common )

if( NOT DAVA_LIBRARY )
    set( DAVA_LIBRARY    "DavaFramework" )
endif()

include ( DavaLibs )

set( MODULE_NAME ${DAVA_LIBRARY} )

set( USE_PARENT_DEFINITIONS true )

set( MODULE_MANAGER true )
set( MODULE_MANAGER_STUB ModuleManager/ModuleManagerStub.cpp )


set( MODULE_TYPE STATIC )#INLINE STATIC DYNAMIC

set( INCLUDES_PRIVATE               ${DAVA_ENGINE_DIR}
                                    ${DAVA_THIRD_PARTY_INCLUDES_PATH}  )

set( INCLUDES_WIN                   $ENV{DXSDK_DIR}/include
                                    ${DAVA_THIRD_PARTY_ROOT_PATH}/include/ffmpeg )


set( FIND_SYSTEM_LIBRARY_IOS        CoreGraphics
                                    Foundation
                                    UIKit
                                    OpenGLES
                                    Metal
                                    OpenAL
                                    CoreAudio
                                    AVFoundation
                                    AudioToolbox
                                    QuartzCore
                                    GameKit
                                    MessageUI
                                    AdSupport
                                    GameController
                                    Security
                                    AssetsLibrary
                                    SystemConfiguration
                                    MediaPlayer
                                    ZLIB
                                    CoreTelephony )

set( FIND_SYSTEM_LIBRARY_MACOS      Cocoa
                                    GLUT
                                    OpenGL
                                    AppKit
                                    AVFoundation
                                    Security
                                    QuartzCore
                                    CoreMedia
                                    WebKit
                                    SystemConfiguration
                                    IOKit
                                    ZLIB
                                    Carbon )


set( FIND_SYSTEM_LIBRARY_ANDROID    ZLIB )

set( STATIC_LIBRARIES_IOS           ${DAVA_STATIC_LIBRARIES_IOS} )
set( STATIC_LIBRARIES_MACOS         ${DAVA_STATIC_LIBRARIES_MACOS} )
set( STATIC_LIBRARIES_ANDROID       ${DAVA_STATIC_LIBRARIES_ANDROID} )
set( STATIC_LIBRARIES_WIN32         ${DAVA_STATIC_LIBRARIES_WIN32} )
set( STATIC_LIBRARIES_WIN32_RELEASE ${DAVA_STATIC_LIBRARIES_WIN32_RELEASE} )
set( STATIC_LIBRARIES_WIN32_DEBUG   ${DAVA_STATIC_LIBRARIES_WIN32_DEBUG} )
set( STATIC_LIBRARIES_WIN64         ${DAVA_STATIC_LIBRARIES_WIN64} )
set( STATIC_LIBRARIES_WIN64_RELEASE ${DAVA_STATIC_LIBRARIES_WIN64_RELEASE} )
set( STATIC_LIBRARIES_WIN64_DEBUG   ${DAVA_STATIC_LIBRARIES_WIN64_DEBUG} )
set( STATIC_LIBRARIES_WINUAP        ${DAVA_STATIC_LIBRARIES_WINUAP} )
set( STATIC_LIBRARIES_WINUAP_RELEASE ${DAVA_STATIC_LIBRARIES_WINUAP_RELEASE} )
set( STATIC_LIBRARIES_WINUAP_DEBUG  ${DAVA_STATIC_LIBRARIES_WINUAP_DEBUG} )


set( DYNAMIC_LIBRARIES_MACOS        ${DAVA_DYNAMIC_LIBRARIES_MACOS})
set( DYNAMIC_LIBRARIES_WIN32        ${DAVA_DYNAMIC_LIBRARIES_WIN32})
set( DYNAMIC_LIBRARIES_WIN64        ${DAVA_DYNAMIC_LIBRARIES_WIN64} )

set( DYNAMIC_LIBRARIES_ANDROID      ${DAVA_DYNAMIC_LIBRARIES_ANDROID} )

set( STATIC_LIBRARIES_LINUX         ${DAVA_STATIC_LIBRARIES_LINUX} )

set( SRC_FOLDERS                    "." )

set( CPP_FILES                      ${DAVA_EXTERNAL_DIR}/lz4/*.c
                                    ${DAVA_EXTERNAL_DIR}/imgui/*.cpp)

set( ERASE_FOLDERS_IOS              "DX9" "DX11" )
set( ERASE_FOLDERS_MACOS            "Metal" "DX9" "DX11" )
set( ERASE_FOLDERS_ANDROID          "Metal" "DX9" "DX11" )
set( ERASE_FOLDERS_WIN              "Metal" )
set( ERASE_FOLDERS_WINUAP           "DX9" "GLES2" "Metal" )
set( ERASE_FOLDERS_LINUX            "DX9" "DX11" "Metal" "GLES2" )
set( ERASE_FILES                    ${MODULE_MANAGER_STUB} )

set( UNITY_IGNORE_LIST              "Internal/Autotesting"
                                    "Compression/ZipCompressor.cpp"
                                    "Platform/Steam.cpp"
                                    "DataStorage/Steam/DataStorageSteam.cpp"
                                    "UI/Private/CEF/WebViewControl.cpp"
                                    "UI/Private/Win32/FfmpegPlayer.Win32.cpp"
                                    "UI/Private/Win32/MovieViewControl.Win32.cpp"
                                    "External/imgui")

set( UNITY_IGNORE_LIST_APPLE        "DataStorage/Apple/ICloudKeyValue.cpp"
                                    "Concurrency/ThreadPosix.cpp"
                                    "Render/Texture.cpp"
                                    "Render/RHI/GLES2"
                                    "Logger/TeamcityOutput.cpp" )

set( UNITY_IGNORE_LIST_IOS          "DataStorage/Apple/ICloudKeyValue.cpp"
                                    "TemplateiOS"
                                    )

set( DEFINITIONS_PRIVATE            -D_UNICODE
                                    -DUNICODE
                                    -D_CRT_SECURE_NO_DEPRECATE
                                    -D_CRT_NON_CONFORMING_SWPRINTFS
)

# Define __STDC_FORMAT_MACROS for PRIdXX macros used in printf and friends
set( DEFINITIONS_PRIVATE_LINUX      -D__STDC_FORMAT_MACROS=1)

set( DEFINITIONS_PRIVATE_IOS        -DDDARWIN_NO_CARBON )
set( DEFINITIONS_PRIVATE_MACOS      -D__DAVAENGINE_PTHREAD__ )
set( DEFINITIONS_PRIVATE_ANDROID    -D__ANDROID__ )
set( DEFINITIONS_PRIVATE_WINUAP     -DSQLITE_OS_WINRT=1 )
set( DEFINITIONS_PRIVATE_WIN        -DDISABLE_NATIVE_WEBVIEW )

#
module_options( DAVA_DISABLE_AUTOTESTS )
module_options( DAVA_USE_RENDERSTATS )
module_options( DAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME )
module_options( DENY_RUN_MULTIPLE_APP_INSTANCES )
module_options( ANDROID_USE_LOCAL_RESOURCES )
module_options( DAVA_PLATFORM_QT )
module_options( DAVA_LOCALIZATION_DEBUG )


if( DAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME )
    list( APPEND DEFINITIONS -DDAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME )
endif()

if (DAVA_PLATFORM_QT)
    find_package(QT5 REQUIRED Core Gui Widgets QuickWidgets Network)

    list( APPEND DEFINITIONS    -D__DAVAENGINE_QT__
                                -DDISABLE_NATIVE_WEBVIEW
                                -DDISABLE_NATIVE_TEXTFIELD
                                -DDISABLE_NATIVE_MOVIEVIEW )

endif()

if( DAVA_DISABLE_AUTOTESTS  )
    list( APPEND DEFINITIONS_PRIVATE -DDISABLE_AUTOTESTS )
endif()

if ( STEAM_SDK_FOUND )#tmp
    list( APPEND INCLUDES ${STEAM_SDK_HEADERS} )
    list( APPEND DEFINITIONS_PRIVATE -D__DAVAENGINE_STEAM__ )
endif ()

if( DAVA_USE_RENDERSTATS )
    list( APPEND DEFINITIONS_PRIVATE -D__DAVAENGINE_RENDERSTATS__ )
endif()

if( DENY_RUN_MULTIPLE_APP_INSTANCES )
    list( APPEND DEFINITIONS_PRIVATE -DDENY_RUN_MULTIPLE_APP_INSTANCES )
endif()

if ( ANDROID_USE_LOCAL_RESOURCES )
    list( APPEND DEFINITIONS_PRIVATE -DUSE_LOCAL_RESOURCES )
endif()

if (NOT DEPLOY)
    list( APPEND DEFINITIONS -DLOCAL_FRAMEWORK_SOURCE_PATH="${DAVA_ROOT_DIR}" )
endif()

if( DAVA_LOCALIZATION_DEBUG  )
    list( APPEND DEFINITIONS -DLOCALIZATION_DEBUG )
endif()

setup_main_module()

