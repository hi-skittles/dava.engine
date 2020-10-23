cmake_minimum_required(VERSION 3.4)
project(jpeg)

set (SOURCE_FILES
    "jaricom.c"
    "jcapimin.c"
    "jcapistd.c"
    "jcarith.c"
    "jccoefct.c"
    "jccolor.c"
    "jcdctmgr.c"
    "jchuff.c"
    "jcinit.c"
    "jcmainct.c"
    "jcmarker.c"
    "jcmaster.c"
    "jcomapi.c"
    "jcparam.c"
    "jcprepct.c"
    "jcsample.c"
    "jctrans.c"
    "jdapimin.c"
    "jdapistd.c"
    "jdarith.c"
    "jdatadst.c"
    "jdatasrc.c"
    "jdcoefct.c"
    "jdcolor.c"
    "jddctmgr.c"
    "jdhuff.c"
    "jdinput.c"
    "jdmainct.c"
    "jdmarker.c"
    "jdmaster.c"
    "jdmerge.c"
    "jdpostct.c"
    "jdsample.c"
    "jdtrans.c"
    "jerror.c"
    "jfdctflt.c"
    "jfdctfst.c"
    "jfdctint.c"
    "jidctflt.c"
    "jidctfst.c"
    "jidctint.c"
    "jmemmgr.c"
    "jmemnobs.c"
    "jquant1.c"
    "jquant2.c"
    "jutils.c"
)
add_library(jpeg STATIC ${SOURCE_FILES})

if(APPLE)
  if(IOS)
    set( CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY iPhone/iPad )
    set( CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 7.0)
  else()
    set( CMAKE_OSX_DEPLOYMENT_TARGET 10.8 )
    set( CMAKE_OSX_ARCHITECTURES x86_64;i386 )
  endif()
endif()
