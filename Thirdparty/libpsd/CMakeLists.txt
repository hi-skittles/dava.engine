cmake_minimum_required(VERSION 3.4)
project(psd)

include_directories(./include)

if(WIN32)
  find_package(ZLIB)
  include_directories(${ZLIB_INCLUDE_DIR})
endif()

FILE(GLOB SOURCE_FILES src/*.c)
add_library(psd STATIC ${SOURCE_FILES})

if(APPLE)
  if(IOS)
    set( CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY iPhone/iPad )
    set( CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 7.0)
  else()
    set( CMAKE_OSX_DEPLOYMENT_TARGET 10.8 )
    set( CMAKE_OSX_ARCHITECTURES x86_64;i386 )
  endif()
endif()
