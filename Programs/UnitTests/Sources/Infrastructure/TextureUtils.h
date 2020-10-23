#pragma once

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"

namespace DAVA
{
class Image;
class Sprite;
}

class TextureUtils
{
public:
    struct CompareResult
    {
        DAVA::uint32 difference;
        DAVA::uint32 bytesCount;
    };

    static CompareResult CompareImages(const DAVA::Image* first, const DAVA::Image* second, DAVA::PixelFormat format);
};
