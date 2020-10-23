cmake_minimum_required(VERSION 3.4)
project(mongodb)

include_directories(./src)

FILE(GLOB SOURCE_FILES src/*.c)

if(NOT WIN32)
  list(REMOVE_ITEM SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/env_win32.c")
else()
  list(REMOVE_ITEM SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/env_posix.c")
  list(REMOVE_ITEM SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/env_standard.c")
endif()

add_library(mongodb STATIC ${SOURCE_FILES})

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
  if(IOS)
    set( CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY iPhone/iPad )
    set( CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 7.0)
  else()
    set( CMAKE_OSX_DEPLOYMENT_TARGET 10.8 )
    set( CMAKE_OSX_ARCHITECTURES x86_64;i386 )
  endif()
endif()
