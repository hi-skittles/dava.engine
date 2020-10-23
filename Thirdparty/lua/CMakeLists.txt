cmake_minimum_required(VERSION 3.4)
project(lua)

include_directories(./src)

# Explicitly specify source files for lua static library
set (SOURCE_FILES
    # core files
    "src/bit.c"
    "src/lapi.c"
    "src/lcode.c"
    "src/ldebug.c"
    "src/ldo.c"
    "src/ldump.c"
    "src/lfunc.c"
    "src/lgc.c"
    "src/llex.c"
    "src/lmem.c"
    "src/lobject.c"
    "src/lopcodes.c"
    "src/lparser.c"
    "src/lstate.c"
    "src/lstring.c"
    "src/ltable.c"
    "src/ltm.c"
    "src/lundump.c"
    "src/lvm.c"
    "src/lzio.c"
    # lib files
    "src/lauxlib.c"
    "src/lbaselib.c"
    "src/ldblib.c"
    "src/liolib.c"
    "src/lmathlib.c"
    "src/loslib.c"
    "src/ltablib.c"
    "src/lstrlib.c"
    "src/loadlib.c"
    "src/linit.c"
)
add_library(lua STATIC ${SOURCE_FILES})

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
