#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)

// Types from Windows SDK headers
typedef UINT MMRESULT;

typedef struct timecaps_tag
{
    UINT wPeriodMin; /* minimum period supported  */
    UINT wPeriodMax; /* maximum period supported  */
} TIMECAPS, *PTIMECAPS, NEAR *NPTIMECAPS, FAR *LPTIMECAPS;

// To use with MapVirtualKey
#if !defined(MAPVK_VK_TO_VSC)
#define MAPVK_VK_TO_VSC (0)
#define MAPVK_VSC_TO_VK (1)
#define MAPVK_VK_TO_CHAR (2)
#define MAPVK_VSC_TO_VK_EX (3)
#define MAPVK_VK_TO_VSC_EX (4)
#endif

namespace DAVA
{
namespace Private
{
// Static class that holds pointers to functions exported from DLLs.
// Windows API provides some useful functions which are not available for Universal Windows Platfrom,
// but those functions are present in system DLLs.
struct DllImport
{
    static void Initialize();

    // Kernel functions
    static HMODULE(WINAPI* fnLoadLibraryW)(LPCWSTR lpLibFileName);

    // Time API functions
    static MMRESULT(WINAPI* fnTimeGetDevCaps)(LPTIMECAPS ptc, UINT cbtc);
    static MMRESULT(WINAPI* fnTimeBeginPeriod)(UINT uPeriod);
    static MMRESULT(WINAPI* fnTimeEndPeriod)(UINT uPeriod);

    static UINT(WINAPI* fnMapVirtualKey)(UINT uCode, UINT uMapType);
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
