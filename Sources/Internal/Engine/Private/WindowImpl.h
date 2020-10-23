#pragma once

#if defined(__DAVAENGINE_QT__)
#include "Engine/Private/Qt/WindowImplQt.h"
#elif defined(__DAVAENGINE_WIN32__)
#include "Engine/Private/Win32/WindowImplWin32.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
#include "Engine/Private/Win10/WindowImplWin10.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "Engine/Private/Mac/WindowImplMac.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "Engine/Private/Ios/WindowImplIos.h"
#elif defined(__DAVAENGINE_ANDROID__)
#include "Engine/Private/Android/WindowImplAndroid.h"
#elif defined(__DAVAENGINE_LINUX__)
#include "Engine/Private/Linux/WindowImplLinux.h"
#else
#error "WindowImpl is not implemented"
#endif
