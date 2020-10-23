#pragma once

#include "DAVAEngine.h"

class EditorHeightmap : public DAVA::Heightmap
{
    static const DAVA::int32 MAX_EDITOR_HEIGHTMAP_SIZE = 512;
    static const DAVA::int32 VALUE_NOT_CHANGED = 0;
    static const DAVA::int32 VALUE_WAS_CHANGED = 1;

public:
    EditorHeightmap(DAVA::Heightmap* heightmap);
    virtual ~EditorHeightmap();

    void HeghtWasChanged(const DAVA::Rect& changedRect);

    void DrawRelativeRGBA(DAVA::Image* src, DAVA::int32 x, DAVA::int32 y, DAVA::int32 width, DAVA::int32 height, DAVA::float32 k);
    void DrawAverageRGBA(DAVA::Image* mask, DAVA::int32 x, DAVA::int32 y, DAVA::int32 width, DAVA::int32 height, DAVA::float32 k);
    void DrawAbsoluteRGBA(DAVA::Image* mask, DAVA::int32 x, DAVA::int32 y, DAVA::int32 width, DAVA::int32 height, DAVA::float32 time, DAVA::float32 dstHeight);
    void DrawCopypasteRGBA(DAVA::Image* mask, const DAVA::Vector2& posFrom, const DAVA::Vector2& posTo, DAVA::int32 width, DAVA::int32 height, DAVA::float32 koef);

    static void DrawCopypasteRGBA(DAVA::Image* src, DAVA::Image* dst, DAVA::Image* mask, const DAVA::Vector2& posFrom, const DAVA::Vector2& posTo, DAVA::int32 width, DAVA::int32 height);

    static bool Clipping(DAVA::int32& srcOffset, DAVA::int32& dstOffset, DAVA::int32& dstX, DAVA::int32& dstY,
                         DAVA::int32 dstWidth, DAVA::int32 dstHeight, DAVA::int32& width, DAVA::int32& height,
                         DAVA::int32& yAddSrc, DAVA::int32& xAddDst, DAVA::int32& yAddDst);

protected:
    void DownscaleOrClone();
    void Downscale(DAVA::int32 newSize);
    void Upscale();
    void InitializeScalingTable(DAVA::int32 count);

    void InitializeTableOfChanges();

    DAVA::uint16 GetHeightValue(DAVA::int32 posX, DAVA::int32 posY, DAVA::int32 muliplier);
    DAVA::uint16 GetVerticalValue(DAVA::int32 posY, DAVA::int32 muliplier);
    DAVA::uint16 GetHorizontalValue(DAVA::int32 posX, DAVA::int32 muliplier);

    void UpscaleValue(DAVA::int32 leftX, DAVA::int32 topY, DAVA::int32 muliplier);

protected:
    Heightmap* savedHeightmap = nullptr;

    DAVA::uint8* tableOfChanges = nullptr;
    DAVA::float32* scalingTable = nullptr;
};
