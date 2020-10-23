#ifndef __DAVAENGINE_IMAGEPACKER_H__
#define __DAVAENGINE_IMAGEPACKER_H__

#include "Base/BaseTypes.h"
#include "Math/Math2D.h"

namespace DAVA
{
enum class PackingAlgorithm
{
    ALG_BASIC,
    ALG_MAXRECTS_BOTTOM_LEFT,
    ALG_MAXRECTS_BEST_AREA_FIT,
    ALG_MAXRECTS_BEST_SHORT_SIDE_FIT,
    ALG_MAXRECTS_BEST_LONG_SIDE_FIT,
    ALG_MAXRRECT_BEST_CONTACT_POINT
};

struct SpriteBoundsRect
{
    Rect2i marginsRect;
    Rect2i spriteRect;
    uint32 leftEdgePixel = 0;
    uint32 rightEdgePixel = 0;
    uint32 topEdgePixel = 0;
    uint32 bottomEdgePixel = 0;
    uint32 rightMargin = 0;
    uint32 bottomMargin = 0;
};

struct SpritesheetLayout
{
    virtual ~SpritesheetLayout() = default;
    virtual bool AddSprite(const Size2i& spriteSize, const void* searchPtr) = 0;
    virtual const SpriteBoundsRect* GetSpriteBoundsRect(const void* searchPtr) const = 0;
    virtual const Rect2i& GetRect() const = 0;
    virtual uint32 GetWeight() const = 0;

    static std::unique_ptr<SpritesheetLayout> Create(uint32 w, uint32 h, bool duplicateEdgePixel, uint32 spritesMargin, PackingAlgorithm alg);
};
}

#endif // __DAVAENGINE_IMAGEPACKER_H__
