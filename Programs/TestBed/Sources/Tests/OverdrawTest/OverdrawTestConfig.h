#pragma once

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"

namespace OverdrawPerformanceTester
{
struct FrameData
{
    DAVA::float32 FrameTime;
    DAVA::float32 Overdraw;
};
}

namespace OverdrawTestConfig
{
extern DAVA::float32 chartHeight;
extern DAVA::uint16 textureResolution;
extern DAVA::uint8 overdrawScreensCount;
extern DAVA::PixelFormat pixelFormat;
}