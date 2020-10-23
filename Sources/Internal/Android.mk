#-----------------------------
# Framework lib

# set local path for lib
LOCAL_PATH := $(call my-dir)
LOCAL_MODULES_PATH := $(LOCAL_PATH)/../../Modules

# HACK for right order of .so linking
# c++_shared must be linked before all other shared libs, so add it manually
# if we don't do it, linker can take symbols from wrong shared lib (unwind_backtrace from fmod, for example)
ifeq ($(APP_STL), c++_shared)
# Yet another hack - we don't need gcc lib, so unset variable with option -lgcc
TARGET_LIBGCC :=

include $(CLEAR_VARS)
LOCAL_MODULE := cxx-shared-prebuild
LOCAL_SRC_FILES := $(NDK_ROOT)/sources/cxx-stl/llvm-libc++/$(LLVM_VERSION)/libs/$(TARGET_ARCH_ABI)/libc++_shared.so
include $(PREBUILT_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := xml
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libxml.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := png
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libpng.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := yaml
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libyaml.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mongodb
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libmongodb.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := lua
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/liblua.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dxt
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libdxt.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := jpeg
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libjpeg.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ssl
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libssl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := zip
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libzip.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := icucommon
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libicucommon.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := unibreak
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libunibreak.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := uv
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libuv.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := webp
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libwebp.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sqlite3
LOCAL_SRC_FILES := ../../Libs/lib_CMake/android/$(TARGET_ARCH_ABI)/libsqlite3.a
include $(PREBUILT_STATIC_LIBRARY)

# Modules libraries
include $(CLEAR_VARS)
LOCAL_MODULE := fmodex-prebuild
LOCAL_SRC_FILES := ../../Modules/Sound/Libs/Android/$(TARGET_ARCH_ABI)/libfmodex.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := fmodevent-prebuild
LOCAL_SRC_FILES := ../../Modules/Sound/Libs/Android/$(TARGET_ARCH_ABI)/libfmodevent.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := spine
LOCAL_SRC_FILES := ../../Modules/Spine/Libs/Android/$(TARGET_ARCH_ABI)/spine.a
include $(PREBUILT_STATIC_LIBRARY)

DAVA_ROOT := $(LOCAL_PATH)

# set path for includes
DV_LOCAL_C_INCLUDES := $(LOCAL_PATH)
DV_LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Tools/
DV_LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../Libs/include
DV_LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../Libs/lua/include
DV_LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../Libs/icucommon/source/common
DV_LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../Libs/openssl/include/android
# Modules includes
DV_LOCAL_C_INCLUDES += $(LOCAL_MODULES_PATH)/Sound/Libs/Include
DV_LOCAL_C_INCLUDES += $(LOCAL_MODULES_PATH)/Sound/Sources
DV_LOCAL_C_INCLUDES += $(LOCAL_MODULES_PATH)/Spine/Libs/Include
DV_LOCAL_C_INCLUDES += $(LOCAL_MODULES_PATH)/Spine/Sources

# set exported includes
DV_LOCAL_EXPORT_C_INCLUDES := $(DV_LOCAL_C_INCLUDES)

DV_LOCAL_ARM_NEON := true
DV_LOCAL_ARM_MODE := arm
DV_LOCAL_NEON_CFLAGS := -mfloat-abi=softfp -mfpu=neon -march=armv7
DV_LOCAL_CFLAGS += -DUSE_NEON

# set build flags
DV_LOCAL_CPPFLAGS += -frtti -DGL_GLEXT_PROTOTYPES=1
DV_LOCAL_CPPFLAGS += -std=c++14

DV_LOCAL_CFLAGS += -D__DAVAENGINE_ANDROID__
DV_LOCAL_CFLAGS += -D__DAVAENGINE_POSIX__

# remove warnings about unused arguments to compiler
DV_LOCAL_CFLAGS += -Qunused-arguments
# enable ALL warnings
# we write C++ so check warnings only inside our C++ code not C libraries
DV_LOCAL_CPPFLAGS += -Weverything
# treat warnings as errors
DV_LOCAL_CPPFLAGS += -Werror
# disable common simple warnings
# read about any clang warning messages http://fuckingclangwarnings.com/
DV_LOCAL_CPPFLAGS += -Wno-c++98-compat-pedantic
DV_LOCAL_CPPFLAGS += -Wno-newline-eof
DV_LOCAL_CPPFLAGS += -Wno-gnu-anonymous-struct
DV_LOCAL_CPPFLAGS += -Wno-nested-anon-types
DV_LOCAL_CPPFLAGS += -Wno-float-equal
DV_LOCAL_CPPFLAGS += -Wno-extra-semi
DV_LOCAL_CPPFLAGS += -Wno-unused-parameter
DV_LOCAL_CPPFLAGS += -Wno-shadow
DV_LOCAL_CPPFLAGS += -Wno-exit-time-destructors
DV_LOCAL_CPPFLAGS += -Wno-documentation
DV_LOCAL_CPPFLAGS += -Wno-global-constructors
DV_LOCAL_CPPFLAGS += -Wno-padded
DV_LOCAL_CPPFLAGS += -Wno-weak-vtables
DV_LOCAL_CPPFLAGS += -Wno-variadic-macros
DV_LOCAL_CPPFLAGS += -Wno-deprecated-register
DV_LOCAL_CPPFLAGS += -Wno-sign-conversion
DV_LOCAL_CPPFLAGS += -Wno-sign-compare
DV_LOCAL_CPPFLAGS += -Wno-format-nonliteral

# TODO fix next warnings first
DV_LOCAL_CPPFLAGS += -Wno-cast-align
DV_LOCAL_CPPFLAGS += -Wno-conversion
DV_LOCAL_CPPFLAGS += -Wno-unreachable-code
DV_LOCAL_CPPFLAGS += -Wno-zero-length-array
DV_LOCAL_CPPFLAGS += -Wno-switch-enum
DV_LOCAL_CPPFLAGS += -Wno-c99-extensions
DV_LOCAL_CPPFLAGS += -Wno-missing-prototypes
DV_LOCAL_CPPFLAGS += -Wno-missing-field-initializers
DV_LOCAL_CPPFLAGS += -Wno-conditional-uninitialized
DV_LOCAL_CPPFLAGS += -Wno-covered-switch-default
DV_LOCAL_CPPFLAGS += -Wno-deprecated
DV_LOCAL_CPPFLAGS += -Wno-unused-macros
DV_LOCAL_CPPFLAGS += -Wno-disabled-macro-expansion
DV_LOCAL_CPPFLAGS += -Wno-undef
DV_LOCAL_CPPFLAGS += -Wno-non-virtual-dtor
DV_LOCAL_CPPFLAGS += -Wno-char-subscripts
DV_LOCAL_CPPFLAGS += -Wno-unneeded-internal-declaration
DV_LOCAL_CPPFLAGS += -Wno-unused-variable
DV_LOCAL_CPPFLAGS += -Wno-used-but-marked-unused
DV_LOCAL_CPPFLAGS += -Wno-missing-variable-declarations
DV_LOCAL_CPPFLAGS += -Wno-gnu-statement-expression
DV_LOCAL_CPPFLAGS += -Wno-missing-braces
DV_LOCAL_CPPFLAGS += -Wno-reorder
DV_LOCAL_CPPFLAGS += -Wno-implicit-fallthrough
DV_LOCAL_CPPFLAGS += -Wno-ignored-qualifiers
DV_LOCAL_CPPFLAGS += -Wno-shift-sign-overflow
DV_LOCAL_CPPFLAGS += -Wno-mismatched-tags
DV_LOCAL_CPPFLAGS += -Wno-missing-noreturn
DV_LOCAL_CPPFLAGS += -Wno-consumed
DV_LOCAL_CPPFLAGS += -Wno-sometimes-uninitialized
DV_LOCAL_CPPFLAGS += -Wno-reserved-id-macro
DV_LOCAL_CPPFLAGS += -Wno-old-style-cast
# we have to do it because clang3.6 bug http://bugs.mitk.org/show_bug.cgi?id=18883
DV_LOCAL_CPPFLAGS += -Wno-error=inconsistent-missing-override
DV_LOCAL_CPPFLAGS += -Wno-inconsistent-missing-override
DV_LOCAL_CPPFLAGS += -Wno-null-conversion
DV_LOCAL_CPPFLAGS += -Wno-unused-local-typedef
DV_LOCAL_CPPFLAGS += -Wno-unreachable-code-return
DV_LOCAL_CPPFLAGS += -Wno-unreachable-code-break
DV_LOCAL_CPPFLAGS += -Wno-unknown-warning-option
DV_LOCAL_CPPFLAGS += -Wno-pedantic
DV_LOCAL_CPPFLAGS += -Wno-extern-c-compat
DV_LOCAL_CPPFLAGS += -Wno-unknown-pragmas
DV_LOCAL_CPPFLAGS += -Wno-unused-private-field
DV_LOCAL_CPPFLAGS += -Wno-unused-label
DV_LOCAL_CPPFLAGS += -Wno-unused-function
DV_LOCAL_CPPFLAGS += -Wno-unused-value
DV_LOCAL_CPPFLAGS += -Wno-self-assign-field
DV_LOCAL_CPPFLAGS += -Wno-undefined-reinterpret-cast

# These warnings were added after switch on ndk 12
DV_LOCAL_CPPFLAGS += -Wno-documentation-unknown-command
DV_LOCAL_CPPFLAGS += -Wno-double-promotion
DV_LOCAL_CPPFLAGS += -Wno-over-aligned
DV_LOCAL_CPPFLAGS += -Wno-shift-negative-value

# These warnings were added after switch on crystax ndk
DV_LOCAL_CPPFLAGS += -Wno-macro-redefined
DV_LOCAL_CPPFLAGS += -Wno-format

DV_LOCAL_CPP_FEATURES += exceptions

ifeq ($(DAVA_PROFILE), true)
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
$(info ==============)
$(info profiling enabled!)
$(info ==============)

DV_LOCAL_CFLAGS += -pg
DV_LOCAL_CFLAGS += -D__DAVAENGINE_PROFILE__
endif
endif

DV_LOCAL_CFLAGS += -fno-standalone-debug

# set exported build flags
DV_LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS) -fno-standalone-debug
DV_LOCAL_EXPORT_CFLAGS += -D__DAVAENGINE_ANDROID__
DV_LOCAL_EXPORT_CFLAGS += -D__DAVAENGINE_POSIX__

# set exported used libs
DV_LOCAL_EXPORT_LDLIBS := $(LOCAL_LDLIBS)

# set exported linker flags
# ld.bfd doesn't fail with big *.a and *.o
DV_LOCAL_EXPORT_LDFLAGS := -fuse-ld=bfd

# set included libraries
DV_LOCAL_STATIC_LIBRARIES := libc++abi
DV_LOCAL_STATIC_LIBRARIES += liblz4
DV_LOCAL_STATIC_LIBRARIES += libimgui

ifeq ($(APP_STL), c++_shared)
DV_LOCAL_SHARED_LIBRARIES += cxx-shared-prebuild
endif

DV_LOCAL_SHARED_LIBRARIES += fmodex-prebuild
DV_LOCAL_SHARED_LIBRARIES += fmodevent-prebuild

ifeq ($(DAVA_PROFILE), true)
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
DV_LOCAL_STATIC_LIBRARIES += android-ndk-profiler
endif
endif

DV_LOCAL_STATIC_LIBRARIES += xml
DV_LOCAL_STATIC_LIBRARIES += png
DV_LOCAL_STATIC_LIBRARIES += freetype
DV_LOCAL_STATIC_LIBRARIES += yaml
DV_LOCAL_STATIC_LIBRARIES += mongodb
DV_LOCAL_STATIC_LIBRARIES += lua
DV_LOCAL_STATIC_LIBRARIES += dxt
DV_LOCAL_STATIC_LIBRARIES += jpeg
DV_LOCAL_STATIC_LIBRARIES += curl
DV_LOCAL_STATIC_LIBRARIES += ssl
DV_LOCAL_STATIC_LIBRARIES += crypto
DV_LOCAL_STATIC_LIBRARIES += zip
DV_LOCAL_STATIC_LIBRARIES += icucommon
DV_LOCAL_STATIC_LIBRARIES += unibreak
DV_LOCAL_STATIC_LIBRARIES += uv
DV_LOCAL_STATIC_LIBRARIES += webp
DV_LOCAL_STATIC_LIBRARIES += cpufeatures
DV_LOCAL_STATIC_LIBRARIES += sqlite3

# Modules libraries
DV_LOCAL_STATIC_LIBRARIES += spine

DV_LOCAL_EXPORT_LDLIBS := -lGLESv1_CM -llog -lEGL -latomic -landroid -lz

ifeq ($(APP_PLATFORM), android-14)
	DV_LOCAL_EXPORT_LDLIBS += -lGLESv2
else
ifeq ($(APP_PLATFORM), android-15)
	DV_LOCAL_EXPORT_LDLIBS += -lGLESv2
else
ifeq ($(APP_PLATFORM), android-16)
	DV_LOCAL_EXPORT_LDLIBS += -lGLESv2
else
ifeq ($(APP_PLATFORM), android-17)
	DV_LOCAL_EXPORT_LDLIBS += -lGLESv2
else
	DV_LOCAL_EXPORT_LDLIBS += -lGLESv3
endif
endif
endif
endif

# Modules libpart
# clear all variables
include $(CLEAR_VARS)
# set module name
LOCAL_MODULE := libModules

LOCAL_C_INCLUDES := $(DV_LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(DV_LOCAL_EXPORT_C_INCLUDES)
LOCAL_ARM_NEON := $(DV_LOCAL_ARM_NEON)
LOCAL_ARM_MODE := $(DV_LOCAL_ARM_MODE)
LOCAL_NEON_CFLAGS := $(DV_LOCAL_NEON_CFLAGS)
LOCAL_CPPFLAGS := $(DV_LOCAL_CPPFLAGS)
LOCAL_CFLAGS := $(DV_LOCAL_CFLAGS)
LOCAL_CPP_FEATURES := $(DV_LOCAL_CPP_FEATURES)
LOCAL_EXPORT_CFLAGS := $(DV_LOCAL_EXPORT_CFLAGS)
LOCAL_EXPORT_LDLIBS := $(DV_LOCAL_EXPORT_LDLIBS)
LOCAL_EXPORT_LDFLAGS := $(DV_LOCAL_EXPORT_LDFLAGS)
LOCAL_STATIC_LIBRARIES := $(DV_LOCAL_STATIC_LIBRARIES)
LOCAL_SHARED_LIBRARIES := $(DV_LOCAL_SHARED_LIBRARIES)

LOCAL_SRC_FILES := \
                     $(subst $(LOCAL_PATH)/,, \
                     $(wildcard $(LOCAL_MODULES_PATH)/Sound/Sources/Spine/Private/*.cpp) \
                     $(wildcard $(LOCAL_MODULES_PATH)/Spine/Sources/Spine/*.cpp) \
                     $(wildcard $(LOCAL_MODULES_PATH)/Spine/Sources/UI/*.cpp) \
                     $(wildcard $(LOCAL_MODULES_PATH)/Spine/Sources/UI/Spine/*.cpp) \
                     $(wildcard $(LOCAL_MODULES_PATH)/Spine/Sources/UI/Spine/Private/*.cpp))

include $(BUILD_STATIC_LIBRARY)

# clear all variables
include $(CLEAR_VARS)

# set module name
LOCAL_MODULE := libInternalPart1

# On arm architectures add sysroot option to be able to use
# _Unwind_Backtrace and _Unwind_GetIP for collecting backtraces
# TODO: review checking arm arch and $(ANDROID_NDK_ROOT)
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
       DV_LOCAL_CFLAGS += --sysroot=$(ANDROID_NDK_ROOT)/platforms/$(APP_PLATFORM)/arch-arm
endif

LOCAL_C_INCLUDES := $(DV_LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(DV_LOCAL_EXPORT_C_INCLUDES)
LOCAL_ARM_NEON := $(DV_LOCAL_ARM_NEON)
LOCAL_ARM_MODE := $(DV_LOCAL_ARM_MODE)
LOCAL_NEON_CFLAGS := $(DV_LOCAL_NEON_CFLAGS)
LOCAL_CPPFLAGS := $(DV_LOCAL_CPPFLAGS)
LOCAL_CFLAGS := $(DV_LOCAL_CFLAGS)
LOCAL_CPP_FEATURES := $(DV_LOCAL_CPP_FEATURES)
LOCAL_EXPORT_CFLAGS := $(DV_LOCAL_EXPORT_CFLAGS)
LOCAL_EXPORT_LDLIBS := $(DV_LOCAL_EXPORT_LDLIBS)
LOCAL_EXPORT_LDFLAGS := $(DV_LOCAL_EXPORT_LDFLAGS)
LOCAL_STATIC_LIBRARIES := $(DV_LOCAL_STATIC_LIBRARIES)
LOCAL_SHARED_LIBRARIES := $(DV_LOCAL_SHARED_LIBRARIES)

# set source files
LOCAL_SRC_FILES := \
                     $(subst $(LOCAL_PATH)/,, \
                     $(wildcard $(LOCAL_PATH)/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Analytics/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Analytics/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Animation/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Autotesting/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Autotesting/*.c) \
                     $(wildcard $(LOCAL_PATH)/Base/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Base/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Collision/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Core/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Command/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Compression/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Clipboard/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Database/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Debug/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Debug/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DeviceManager/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DeviceManager/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DeviceManager/Private/Android/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DLCManager/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Engine/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Engine/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Engine/Private/Dispatcher/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Engine/Private/Android/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Entity/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Entity/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/FileSystem/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/FileSystem/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Functional/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Input/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Input/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Input/Private/Android/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Math/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Math/Neon/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/MemoryManager/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Network/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Network/Base/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Network/Services/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Network/Services/MMNet/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Network/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Particles/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Particles/ParticleEffectDebug/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Platform/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Platform/TemplateAndroid/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Platform/TemplateAndroid/BacktraceAndroid/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Platform/TemplateAndroid/ExternC/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/PluginManager/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/PluginManager/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Reflection/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Reflection/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Reflection/Private/Wrappers/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/ReflectionDeclaration/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/ReflectionDeclaration/Private/*.cpp))

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

# set module name
LOCAL_MODULE := libInternal

LOCAL_C_INCLUDES := $(DV_LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(DV_LOCAL_EXPORT_C_INCLUDES)
LOCAL_ARM_NEON := $(DV_LOCAL_ARM_NEON)
LOCAL_ARM_MODE := $(DV_LOCAL_ARM_MODE)
LOCAL_NEON_CFLAGS := $(DV_LOCAL_NEON_CFLAGS)
LOCAL_CPPFLAGS := $(DV_LOCAL_CPPFLAGS)
LOCAL_CFLAGS := $(DV_LOCAL_CFLAGS)
LOCAL_CPP_FEATURES := $(DV_LOCAL_CPP_FEATURES)
LOCAL_EXPORT_CFLAGS := $(DV_LOCAL_EXPORT_CFLAGS)
LOCAL_EXPORT_LDLIBS := $(DV_LOCAL_EXPORT_LDLIBS)
LOCAL_EXPORT_LDFLAGS := $(DV_LOCAL_EXPORT_LDFLAGS)
LOCAL_STATIC_LIBRARIES := $(DV_LOCAL_STATIC_LIBRARIES)
LOCAL_SHARED_LIBRARIES := $(DV_LOCAL_SHARED_LIBRARIES)

LOCAL_WHOLE_STATIC_LIBRARIES := libInternalPart1 libModules

LOCAL_SRC_FILES := \
                     $(subst $(LOCAL_PATH)/,, \
                     $(wildcard $(LOCAL_PATH)/ModuleManager/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/2D/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/2D/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/2D/Systems/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/3D/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/Effects/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/Highlevel/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/Highlevel/Vegetation/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/Material/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/RHI/Common/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/RHI/Common/MCPP/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/RHI/Common/Parser/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/RHI/GLES2/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/RHI/NullRenderer/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene2D/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Components/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Components/Controller/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Components/Waypoint/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Converters/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/SceneFile/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/SkeletonAnimation/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/SkeletonAnimation/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Systems/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Systems/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Systems/Controller/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Lod/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Lod/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scripting/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Sound/Private/SoundSystem.cpp) \
                     $(wildcard $(LOCAL_PATH)/Time/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Time/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Concurrency/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Components/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Styles/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Layouts/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Layouts/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Formula/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Formula/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/DataBinding/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/DataBinding/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Focus/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Input/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/RichContent/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/RichContent/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Scroll/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Sound/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Text/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Text/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Private/Android/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Update/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UI/Render/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/UnitTests/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Utils/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Job/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/Image/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Render/Image/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DLC/Downloader/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DLC/Patcher/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DLC/Patcher/bsdiff/*.c) \
                     $(wildcard $(LOCAL_PATH)/DLC/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DataStorage/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/DataStorage/Android/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Notification/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Notification/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Notification/Private/Android/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/CommandLine/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Logger/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Components/SingleComponents/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/Scene3D/Components/SingleComponents/Private/*.cpp) \
                     $(wildcard $(LOCAL_PATH)/AnyCasts/Private/*.cpp))

include $(BUILD_STATIC_LIBRARY)

# include modules
$(call import-add-path,$(DAVA_ROOT)/..)
$(call import-add-path,$(DAVA_ROOT)/../External)
$(call import-add-path,$(DAVA_ROOT)/../External/lz4)
$(call import-add-path,$(DAVA_ROOT)/../External/imgui)
$(call import-add-path,$(DAVA_ROOT))

ifeq ($(DAVA_PROFILE), true)
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
$(call import-add-path,$(DAVA_ROOT)/../../Libs)
$(call import-module,android-ndk-profiler)
endif
endif

$(call import-module,lz4)
$(call import-module,imgui)
$(call import-module,android/cpufeatures)
