#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN32__)

#include <ShellScalingApi.h>
#include <WinUser.h>

namespace DAVA
{
namespace Private
{
// Static class that holds pointers to functions exported from DLLs.
// Each Windows version introduces some new useful API functions that we would like to use.
// If new API is available then pointers to correspondng functions are not null and dava.engine will
// use that new API.
struct DllImport final
{
    static void Initialize();

    // New pointer input functions, available starting from Windows 8
    static BOOL(WINAPI* fnEnableMouseInPointer)(BOOL fEnable);
    static BOOL(WINAPI* fnIsMouseInPointerEnabled)();
    static BOOL(WINAPI* fnGetPointerInfo)(UINT32 pointerId, POINTER_INFO* pointerInfo);

    // Shell scaling functions
    // GetDpiForMonitor is available starting from Windows Vista
    static HRESULT(STDAPICALLTYPE* fnGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT* dpiX, UINT* dpiY);
    static HRESULT(STDAPICALLTYPE* fnGetProcessDpiAwareness)(HANDLE hprocess, PROCESS_DPI_AWARENESS* value);
    static HRESULT(STDAPICALLTYPE* fnSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);

    // Screen auto-rotation support functions (WinUser.h)
    static BOOL(WINAPI* fnGetAutoRotationState)(PAR_STATE pState);
    static BOOL(WINAPI* fnGetDisplayAutoRotationPreferences)(ORIENTATION_PREFERENCE* pOrientation);
    static BOOL(WINAPI* fnSetDisplayAutoRotationPreferences)(ORIENTATION_PREFERENCE orientation);
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
