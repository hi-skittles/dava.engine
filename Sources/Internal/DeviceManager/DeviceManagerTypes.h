#pragma once

#include "Base/BaseTypes.h"
#include "Math/Rect.h"

namespace DAVA
{
/**
    \ingroup device_manager
    Struct which describes display device.
*/
struct DisplayInfo
{
    uintptr_t systemId = 0;
    Rect rect; //<! Display rect in raw (physical) pixels
    float32 rawDpiX = 0.f; //!< Raw dots per inch along X-axis of display
    float32 rawDpiY = 0.f; //!< Raw dots per inch along Y-axis of display
    uint32 maxFps = 60; //!< Maximum supported FPS for display
    bool primary = false; //!< Is display primary
    String name; //<! Display name as seen by system
};

} // namespace DAVA
