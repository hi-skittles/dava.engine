#ifndef __PAINT_HEIGHT_DELTA_H__
#define __PAINT_HEIGHT_DELTA_H__

#include "DAVAEngine.h"

namespace PaintHeightDelta
{
void Execute(const DAVA::FilePath& targetImagePath, DAVA::float32 refDelta, DAVA::Heightmap* srcHeightmap,
             DAVA::uint32 targetImageWidth, DAVA::uint32 targetImageHeight, DAVA::float32 targetTerrainHeight,
             const DAVA::Vector<DAVA::Color>& pixelColors);
}

#endif
