#-----------------------------
# imgui lib

# set local path for lib
LOCAL_PATH := $(call my-dir)

# clear all variables
include $(CLEAR_VARS)

# set module name
LOCAL_MODULE := libimgui

# set path for includes
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/..

# set exported includes
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := imgui.cpp imgui_demo.cpp imgui_draw.cpp

LOCAL_CFLAGS := -O2

ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard),)
LOCAL_ARM_NEON := true
LOCAL_ARM_MODE := arm
LOCAL_NEON_CFLAGS := -mfloat-abi=softfp -mfpu=neon -march=armv7
endif

# build static library
include $(BUILD_STATIC_LIBRARY)
