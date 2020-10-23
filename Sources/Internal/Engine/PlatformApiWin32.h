#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
#elif defined(__DAVAENGINE_WIN32__)

#include "Base/Platform.h"

/**
    \defgroup engine_win32 Engine facilities specific to Win32 platform
*/

namespace DAVA
{
class Window;
namespace PlatformApi
{
namespace Win32
{
/**
    \ingroup engine_win32
    Set window icon stored in resources by its id
*/
void SetWindowIcon(Window* targetWindow, int32 iconResourceId);

/**
    \ingroup engine_win32
    Set window cursor.

    If hcursor is null then restores default cursor (usually arrow).
*/
void SetWindowCursor(Window* targetWindow, HCURSOR hcursor);

} // namespace Win32
} // namespace PlatformApi
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
