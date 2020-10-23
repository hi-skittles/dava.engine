#pragma once

#ifndef DISABLE_AUTOTESTS
#ifndef __DAVAENGINE_AUTOTESTING__
#define __DAVAENGINE_AUTOTESTING__
#endif
#ifndef AUTOTESTING_LUA
#define AUTOTESTING_LUA
#endif
#endif

// Enable memory profiling - should be defined for non-cmake projects
//#define DAVA_MEMORY_PROFILING_ENABLE

//#define ENABLE_BASE_OBJECT_CHECKS // separate thing to check if you release BaseObjects properly. Needs to be disabled for release configurations

//#define ENABLE_CONTROL_EDIT //allows to drag'n'drop controls for position editing

//#define SHOW_FRAME_TIME	// shows milliseconds per fame

//#define LOCALIZATION_DEBUG // enable graphic debugging info for displaying of text

// This flag allows to enable profiling stats
//#define __DAVAENGINE_ENABLE_DEBUG_STATS__
#if defined(__DAVAENGINE_ENABLE_DEBUG_STATS__)
//    #define __DAVAENGINE_ENABLE_FRAMEWORK_STATS__
//    #define __DAVAENGINE_ENABLE_TOOLS_STATS__
#endif //__DAVAENGINE_ENABLE_DEBUG_STATS__

#if defined(__DAVAENGINE_DEBUG__) // always enable full DVASSERT service for debug configurations
#define ENABLE_ASSERT_LOGGING
#define ENABLE_ASSERT_MESSAGE
#define ENABLE_ASSERT_BREAK
#else
#define __DAVAENGINE_ENABLE_ASSERTS__ // comment if DVASSERT macro should be stripped in release mode
#define ENABLE_ASSERT_LOGGING
#define ENABLE_ASSERT_MESSAGE
#endif

#define USE_FILEPATH_IN_MAP
#ifdef USE_FILEPATH_IN_MAP
#define FILEPATH_MAP_KEY(key) key
#else //#ifdef USE_FILEPATH_IN_MAP
#define FILEPATH_MAP_KEY(key) key.GetAbsolutePathname()
#endif //#ifdef USE_FILEPATH_IN_MAP

//Uncomment this define to use C++11 concurrency instead of native
//#define USE_CPP11_CONCURRENCY

//suppress 'deprecated' warning
#define DAVAENGINE_HIDE_DEPRECATED
//suppressing of deprecated functions
#ifdef DAVAENGINE_HIDE_DEPRECATED
#undef DAVA_DEPRECATED
#define DAVA_DEPRECATED(func) func
#endif
