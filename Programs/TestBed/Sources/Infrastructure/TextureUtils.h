#pragma once

#include "DAVAEngine.h"
using namespace DAVA;

class TextureUtils
{
public:
    struct CompareResult
    {
        uint32 difference;
        uint32 bytesCount;
    };

    static Sprite* CreateSpriteFromTexture(const String& texturePathname);
    static CompareResult CompareSprites(Sprite* first, Sprite* second, PixelFormat format);
    static CompareResult CompareImages(Image* first, Image* second, PixelFormat format);
    static Image* CreateImageAsRGBA8888(Sprite* sprite);
};
