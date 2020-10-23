#include "Engine/Private/Win32/DllImportWin32.h"

#if defined(__DAVAENGINE_WIN32__)

namespace DAVA
{
namespace Private
{
// clang-format off
BOOL(WINAPI* DllImport::fnEnableMouseInPointer)(BOOL fEnable);
BOOL(WINAPI* DllImport::fnIsMouseInPointerEnabled)();
BOOL(WINAPI* DllImport::fnGetPointerInfo)(UINT32 pointerId, POINTER_INFO* pointerInfo);
HRESULT(STDAPICALLTYPE* DllImport::fnGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT* dpiX, UINT* dpiY);
HRESULT(STDAPICALLTYPE* DllImport::fnGetProcessDpiAwareness)(HANDLE hprocess, PROCESS_DPI_AWARENESS *value);
HRESULT(STDAPICALLTYPE* DllImport::fnSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);
BOOL(WINAPI* DllImport::fnGetAutoRotationState)(PAR_STATE pState);
BOOL(WINAPI* DllImport::fnGetDisplayAutoRotationPreferences)(ORIENTATION_PREFERENCE *pOrientation);
BOOL(WINAPI* DllImport::fnSetDisplayAutoRotationPreferences)(ORIENTATION_PREFERENCE orientation);
// clang-format on

void DllImport::Initialize()
{
    HMODULE huser32 = ::LoadLibraryW(L"user32");
    if (huser32 != nullptr)
    {
        fnEnableMouseInPointer = reinterpret_cast<decltype(fnEnableMouseInPointer)>(::GetProcAddress(huser32, "EnableMouseInPointer"));
        fnIsMouseInPointerEnabled = reinterpret_cast<decltype(fnIsMouseInPointerEnabled)>(::GetProcAddress(huser32, "IsMouseInPointerEnabled"));
        fnGetPointerInfo = reinterpret_cast<decltype(fnGetPointerInfo)>(::GetProcAddress(huser32, "GetPointerInfo"));

        fnGetAutoRotationState = reinterpret_cast<decltype(fnGetAutoRotationState)>(::GetProcAddress(huser32, "GetAutoRotationState"));
        fnGetDisplayAutoRotationPreferences = reinterpret_cast<decltype(fnGetDisplayAutoRotationPreferences)>(::GetProcAddress(huser32, "GetDisplayAutoRotationPreferences"));
        fnSetDisplayAutoRotationPreferences = reinterpret_cast<decltype(fnSetDisplayAutoRotationPreferences)>(::GetProcAddress(huser32, "SetDisplayAutoRotationPreferences"));
    }

    HMODULE hshcore = ::LoadLibraryW(L"shcore");
    if (hshcore != nullptr)
    {
        fnGetDpiForMonitor = reinterpret_cast<decltype(fnGetDpiForMonitor)>(::GetProcAddress(hshcore, "GetDpiForMonitor"));
        fnGetProcessDpiAwareness = reinterpret_cast<decltype(fnGetProcessDpiAwareness)>(::GetProcAddress(hshcore, "GetProcessDpiAwareness"));
        fnSetProcessDpiAwareness = reinterpret_cast<decltype(fnSetProcessDpiAwareness)>(::GetProcAddress(hshcore, "SetProcessDpiAwareness"));
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
