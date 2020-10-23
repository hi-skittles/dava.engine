#include "Engine/Private/Win10/DllImportWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

namespace DAVA
{
namespace Private
{
// clang-format off
HMODULE(WINAPI* DllImport::fnLoadLibraryW)(LPCWSTR lpLibFileName);
MMRESULT(WINAPI* DllImport::fnTimeGetDevCaps)(LPTIMECAPS ptc, UINT cbtc) = nullptr;
MMRESULT(WINAPI* DllImport::fnTimeBeginPeriod)(UINT uPeriod) = nullptr;
MMRESULT(WINAPI* DllImport::fnTimeEndPeriod)(UINT uPeriod) = nullptr;
UINT(WINAPI* DllImport::fnMapVirtualKey)(UINT uCode, UINT uMapType) = nullptr;

// clang-format on

void DllImport::Initialize()
{
    // Unversal Windows Platform hides functions such as LoadLibrary, etc.
    // Do the following trick to get pointer to LoadLibrary function:
    //  - GetModuleFileName function is permitted to use and resides in kernel DLL
    //  - VirtualQuery given an address returns information of contiguous memory range an address belongs to
    //  - Windows treats address at which module has been loaded as module handle
    //  - knowing that information we can get module handle for kernel DLL and address of LoadLibrary function
    MEMORY_BASIC_INFORMATION bi;
    ::VirtualQuery(static_cast<void*>(&::GetModuleFileNameW), &bi, sizeof(bi));
    HMODULE hkernel = reinterpret_cast<HMODULE>(bi.AllocationBase);

    fnLoadLibraryW = reinterpret_cast<decltype(fnLoadLibraryW)>(::GetProcAddress(hkernel, "LoadLibraryW"));
    if (fnLoadLibraryW != nullptr)
    {
        HMODULE hwinmm = fnLoadLibraryW(L"winmm.dll");
        if (hwinmm != nullptr)
        {
            fnTimeGetDevCaps = reinterpret_cast<decltype(fnTimeGetDevCaps)>(::GetProcAddress(hwinmm, "timeGetDevCaps"));
            fnTimeBeginPeriod = reinterpret_cast<decltype(fnTimeBeginPeriod)>(::GetProcAddress(hwinmm, "timeBeginPeriod"));
            fnTimeEndPeriod = reinterpret_cast<decltype(fnTimeEndPeriod)>(::GetProcAddress(hwinmm, "timeEndPeriod"));
        }

        HMODULE huser32 = fnLoadLibraryW(L"User32.dll");
        if (huser32 != nullptr)
        {
            fnMapVirtualKey = reinterpret_cast<decltype(fnMapVirtualKey)>(::GetProcAddress(huser32, "MapVirtualKeyW"));
        }
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
