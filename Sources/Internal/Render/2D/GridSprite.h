#ifndef __DAVAENGINE_GRID_SPRITE_H__
#define __DAVAENGINE_GRID_SPRITE_H__


#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"

#include "Render/Texture.h"

namespace DAVA
{
class GridSprite : public BaseObject
{
protected:
    ~GridSprite();

public:
    static GridSprite* Create();

    GridSprite();

    void SetGridSize(int32 gridSizeX, int32 gridSizeY);
    void SetJointColor(int32 jointX, int32 jointY, const Vector4& color);

private:
};
};