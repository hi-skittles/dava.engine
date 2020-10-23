#pragma once

#include "DAVAConfig.h"

#if defined(__DAVAENGINE_WINDOWS__)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX // undef macro min and max from windows headers
#endif

#include <Windows.h>
#include <Windowsx.h>

#undef DrawState
#undef GetCommandLine
#undef GetClassName
#undef GetMessage
#undef Yield
#undef ERROR
#undef DELETE
#undef CopyFile
#undef DeleteFile
#undef MessageBox
#undef CreateDirectory
#undef MoveFile

#if !defined(WINAPI_FAMILY_PARTITION) || WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define __DAVAENGINE_WIN_UAP_INCOMPLETE_IMPLEMENTATION__MARKER__
#define __DAVAENGINE_WIN_UAP_INCOMPLETE_IMPLEMENTATION__ DVASSERT(false, "Feature has no implementation or partly implemented");
#endif

//Using C++11 concurrency as default
#if defined(__DAVAENGINE_WIN_UAP__) && !defined(USE_CPP11_CONCURRENCY)
#define USE_CPP11_CONCURRENCY
#endif

#endif //__DAVAENGINE_WINDOWS__
