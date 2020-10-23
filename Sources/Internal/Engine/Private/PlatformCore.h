#pragma once

#if defined(__DAVAENGINE_QT__)
#include "Engine/Private/Qt/PlatformCoreQt.h"
#elif defined(__DAVAENGINE_WIN32__)
#include "Engine/Private/Win32/PlatformCoreWin32.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
#include "Engine/Private/Win10/PlatformCoreWin10.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "Engine/Private/Mac/PlatformCoreMac.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "Engine/Private/Ios/PlatformCoreIos.h"
#elif defined(__DAVAENGINE_ANDROID__)
#include "Engine/Private/Android/PlatformCoreAndroid.h"
#elif defined(__DAVAENGINE_LINUX__)
#include "Engine/Private/Linux/PlatformCoreLinux.h"
#else
#error "PlatformCore is not implemented"
#endif
