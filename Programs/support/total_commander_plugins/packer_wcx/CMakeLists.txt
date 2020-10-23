cmake_minimum_required(VERSION 3.4)

project(double_commander_packer_plugin)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(OUT_PREFIX "_win.wcx64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static -O3")
else()
    set(OUT_PREFIX "_mac.wcx")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -std=c++11 -stdlib=libc++" )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(SOURCE_FILES 
        main.cpp 
        archive.cpp
        lite_archive.cpp
        pack_archive.cpp 
        pack_archive.cpp 
        pack_meta_data.cpp
        lz4.c
        lz4.h 
        archive.h
        lite_archive.h
        pack_archive.h 
        pack_format.h 
        wcxhead.h)

add_library(double_commander_packer_plugin SHARED ${SOURCE_FILES})

set_target_properties(double_commander_packer_plugin PROPERTIES OUTPUT_NAME "dvpk"
        SUFFIX ${OUT_PREFIX}
        PREFIX "")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_custom_command(TARGET double_commander_packer_plugin
        POST_BUILD
        COMMAND "strip" "dvpk${OUT_PREFIX}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
